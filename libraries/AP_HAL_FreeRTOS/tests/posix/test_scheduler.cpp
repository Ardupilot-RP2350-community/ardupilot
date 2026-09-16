/*
 * This file is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <AP_gtest.h>
#include <AP_HAL_FreeRTOS/common/Scheduler.h>
#include <AP_HAL_FreeRTOS/common/Port.h>
#include <chrono>
#include <cstdlib>

using FreeRTOS::Scheduler;
using FreeRTOS::BinarySemaphore;

static const Scheduler::Config config = {
    {"test_main", 131072, 3, 0},
    {"test_timer", 65536, 5, 0},
    {"test_uart", 65536, 4, 0},
    {"test_io", 65536, 2, 0},
    {1, 3, 4, 2, 1, 5, 2, 1, 2, 1, 4, 1, 4, 1},
    4096, 4, 1, 10, true
};

class TestScheduler : public Scheduler
{
public:
    explicit TestScheduler(const Config &cfg = config) : Scheduler(cfg) {}
    using Scheduler::run_timer_processes;
    using Scheduler::run_io_processes;
    unsigned fallback_ms = 0;
    std::atomic<bool> main_ready {false};
    std::atomic<bool> timer_ready {false};
    std::atomic<bool> io_ready {false};
    void delay_microseconds(uint16_t us) override
    {
        vTaskDelay((us + 999U) / 1000U);
    }
    void reboot(bool) override
    {
        abort();
    }
protected:
    void delay_without_scheduler(uint16_t ms) override
    {
        fallback_ms += ms;
    }
    void main_task_init() override
    {
        main_ready = true;
    }
    void timer_task_init() override
    {
        timer_ready = true;
    }
    void io_task_init() override
    {
        set_current_thread_priority(1);
        io_ready = true;
    }
};

static TestScheduler bootstrap;
class TestHAL : public AP_HAL::HAL
{
public:
    TestHAL() : AP_HAL::HAL(nullptr, nullptr, nullptr, nullptr, nullptr,
                                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                &bootstrap, nullptr, nullptr, nullptr, nullptr) {}
    void run(int, char *const[], Callbacks *) const override {}
};
static TestHAL test_hal;
const AP_HAL::HAL &hal = test_hal;
void AP_HAL::init() {}
uint64_t AP_HAL::micros64()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}
uint32_t AP_HAL::millis()
{
    return AP_HAL::micros64() / 1000U;
}
AP_GTEST_PANIC();

static bool fail_create;
static unsigned create_calls;
static uint32_t observed_depth;
static UBaseType_t observed_priority;
extern "C" BaseType_t __real_xTaskCreate(TaskFunction_t, const char *,
        configSTACK_DEPTH_TYPE, void *, UBaseType_t, TaskHandle_t *);
extern "C" BaseType_t __wrap_xTaskCreate(TaskFunction_t entry, const char *name,
        configSTACK_DEPTH_TYPE depth, void *context, UBaseType_t priority, TaskHandle_t *handle)
{
    create_calls++;
    observed_depth = depth;
    observed_priority = priority;
    if (fail_create) {
        return pdFAIL;
    }
    return __real_xTaskCreate(entry, name, depth, context, priority, handle);
}

class SchedulerTest : public ::testing::Test
{
protected:
    TestScheduler scheduler;
    void SetUp() override
    {
        test_hal.scheduler = &scheduler;
    }
    void TearDown() override
    {
        test_hal.scheduler = &bootstrap;
    }
};

struct Counter {
    unsigned calls = 0;
    void invoke()
    {
        calls++;
    }
    AP_HAL::MemberProc proc()
    {
        return FUNCTOR_BIND_MEMBER(&Counter::invoke, void);
    }
};

TEST_F(SchedulerTest, RegistrationCapacityAndDuplicates)
{
    Counter counters[12];
    scheduler.register_timer_process(nullptr);
    for (auto &counter : counters) {
        scheduler.register_timer_process(counter.proc());
        scheduler.register_timer_process(counter.proc());
        scheduler.register_io_process(counter.proc());
    }
    scheduler.run_timer_processes();
    scheduler.run_io_processes();
    for (unsigned i = 0; i < 12; i++) {
        EXPECT_EQ(counters[i].calls, i < 10 ? 2U : 0U);
    }
}

struct Registration {
    TestScheduler &scheduler;
    Counter counter;
    void invoke()
    {
        scheduler.register_timer_process(counter.proc());
    }
};

TEST_F(SchedulerTest, CallbackCanRegisterWithoutDeadlock)
{
    Registration registration {scheduler, {}};
    scheduler.register_timer_process(FUNCTOR_BIND(&registration, &Registration::invoke, void));
    scheduler.run_timer_processes();
    EXPECT_EQ(registration.counter.calls, 0U);
    scheduler.run_timer_processes();
    EXPECT_EQ(registration.counter.calls, 1U);
}

static unsigned failsafe_calls;
static void failsafe()
{
    failsafe_calls++;
}

TEST_F(SchedulerTest, InitializationAndFailsafe)
{
    EXPECT_FALSE(scheduler.is_system_initialized());
    EXPECT_FALSE(scheduler.in_main_thread());
    scheduler.set_system_initialized();
    EXPECT_TRUE(scheduler.is_system_initialized());
    failsafe_calls = 0;
    scheduler.register_timer_failsafe(failsafe, 1000);
    scheduler.run_timer_processes();
    scheduler.run_io_processes();
    EXPECT_EQ(failsafe_calls, 1U);
    scheduler.register_timer_failsafe(nullptr, 1000);
    scheduler.run_timer_processes();
    EXPECT_EQ(failsafe_calls, 1U);
}

TEST_F(SchedulerTest, StackConversionPrioritiesAndFailures)
{
    Counter counter;
    // Inspect requests without starting workers that refer to this local counter.
    fail_create = true;
    EXPECT_FALSE(scheduler.thread_create(counter.proc(), "stack", 16384, Scheduler::PRIORITY_SPI, 1));
    EXPECT_EQ(observed_depth, (16384U * 4 + 4096) / sizeof(StackType_t));
    EXPECT_EQ(observed_priority, 5U);
    EXPECT_FALSE(scheduler.thread_create(counter.proc(), "high", 16384, Scheduler::PRIORITY_MAIN, 127));
    EXPECT_EQ(observed_priority, Scheduler::MAX_PRIO);
    EXPECT_FALSE(scheduler.thread_create(counter.proc(), "low", 16384, Scheduler::PRIORITY_MAIN, -128));
    EXPECT_EQ(observed_priority, Scheduler::MIN_PRIO);
    const unsigned before = create_calls;
    EXPECT_FALSE(scheduler.thread_create(counter.proc(), "overflow", UINT32_MAX, Scheduler::PRIORITY_IO, 0));
    EXPECT_EQ(create_calls, before);
    fail_create = false;
}

TEST_F(SchedulerTest, DelayBeforeSchedulerAndOutsideMain)
{
    vTaskSuspendAll();
    scheduler.delay(4);
    xTaskResumeAll();
    EXPECT_EQ(scheduler.fallback_ms, 4U);
    const TickType_t start = xTaskGetTickCount();
    scheduler.delay(5);
    EXPECT_GE(xTaskGetTickCount() - start, pdMS_TO_TICKS(5));
    EXPECT_FALSE(scheduler.in_delay_callback());
    scheduler.call_delay_cb(); // Safe before a callback is registered.
}

struct Publisher {
    TestScheduler &scheduler;
    Counter (&counters)[10];
    std::atomic<unsigned> &finished;
    void run()
    {
        for (unsigned repeat = 0; repeat < 20; repeat++) {
            for (auto &counter : counters) {
                scheduler.register_timer_process(counter.proc());
                scheduler.register_io_process(counter.proc());
            }
            vTaskDelay(1);
        }
        finished++;
    }
};

TEST_F(SchedulerTest, ConcurrentRegistrationAndExecution)
{
    Counter counters[10];
    std::atomic<unsigned> finished {0};
    Publisher publisher {scheduler, counters, finished};
    const auto proc = FUNCTOR_BIND(&publisher, &Publisher::run, void);
    ASSERT_TRUE(scheduler.thread_create(proc, "publisher1", 16384, Scheduler::PRIORITY_IO, 0));
    ASSERT_TRUE(scheduler.thread_create(proc, "publisher2", 16384, Scheduler::PRIORITY_IO, 0));
    const uint64_t deadline = AP_HAL::micros64() + 1000000U;
    while (finished.load() != 2 && AP_HAL::micros64() < deadline) {
        scheduler.run_timer_processes();
        scheduler.run_io_processes();
        vTaskDelay(1);
    }
    ASSERT_EQ(finished.load(), 2U);
    vTaskDelay(2);
    for (auto &counter : counters) {
        counter.calls = 0;
    }
    scheduler.run_timer_processes();
    scheduler.run_io_processes();
    for (const auto &counter : counters) {
        EXPECT_EQ(counter.calls, 2U);
    }
}

static std::atomic<unsigned> delay_calls {0};
static void delay_callback()
{
    delay_calls++;
}

class LifecycleCallbacks : public AP_HAL::HAL::Callbacks
{
public:
    explicit LifecycleCallbacks(TestScheduler &s) : scheduler(s) {}
    TestScheduler &scheduler;
    BinarySemaphore setup_entered;
    BinarySemaphore setup_release;
    std::atomic<bool> setup_in_main {false};
    std::atomic<bool> hook_before_setup {false};
    std::atomic<unsigned> loops {0};
    std::atomic<unsigned> timer_calls {0};
    std::atomic<unsigned> io_calls {0};
    void timer_tick()
    {
        if (scheduler.timer_ready.load()) {
            timer_calls++;
        }
    }
    void io_tick()
    {
        if (scheduler.io_ready.load()) {
            io_calls++;
        }
    }
    void setup() override
    {
        setup_in_main = scheduler.in_main_thread();
        hook_before_setup = scheduler.main_ready.load();
        scheduler.register_delay_callback(delay_callback, 1);
        scheduler.delay(3);
        setup_entered.signal();
        if (!setup_release.wait_blocking()) {
            abort();
        }
    }
    void loop() override
    {
        loops++;
        scheduler.delay(2);
    }
};

TEST(SchedulerLifecycle, StartsWorkersAndInitializesAfterSetup)
{
    // The scheduler owns persistent workers, so these objects have static lifetime.
    static TestScheduler scheduler;
    static LifecycleCallbacks callbacks(scheduler);
    test_hal.scheduler = &scheduler;
    scheduler.register_timer_process(FUNCTOR_BIND(&callbacks, &LifecycleCallbacks::timer_tick, void));
    scheduler.register_io_process(FUNCTOR_BIND(&callbacks, &LifecycleCallbacks::io_tick, void));
    scheduler.set_callbacks(&callbacks);
    scheduler.init();
    ASSERT_TRUE(callbacks.setup_entered.wait(1000000));
    EXPECT_TRUE(callbacks.setup_in_main.load());
    EXPECT_TRUE(callbacks.hook_before_setup.load());
    EXPECT_FALSE(scheduler.is_system_initialized());
    EXPECT_TRUE(scheduler.timer_ready.load());
    EXPECT_GT(callbacks.timer_calls.load(), 0U);
    EXPECT_GT(delay_calls.load(), 0U);
    EXPECT_FALSE(scheduler.in_main_thread());
    callbacks.setup_release.signal();
    const uint64_t deadline = AP_HAL::micros64() + 1000000U;
    while ((callbacks.io_calls.load() == 0 || callbacks.loops.load() == 0) && AP_HAL::micros64() < deadline) {
        vTaskDelay(1);
    }
    EXPECT_TRUE(scheduler.is_system_initialized());
    EXPECT_GT(callbacks.loops.load(), 0U);
    EXPECT_TRUE(scheduler.io_ready.load());
    EXPECT_GT(callbacks.io_calls.load(), 0U);
    TaskHandle_t io = xTaskGetHandle("test_io");
    ASSERT_NE(io, nullptr);
    EXPECT_EQ(uxTaskPriorityGet(io), 1U);
    EXPECT_NE(xTaskGetHandle("test_uart"), nullptr);
}

static void run_tests(void *)
{
    const int result = RUN_ALL_TESTS();
    fflush(stdout);
    std::exit(result);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    bootstrap.delay(3);
    if (bootstrap.fallback_ms != 3 ||
        xTaskCreate(run_tests, "tests", 32768, nullptr, 7, nullptr) != pdPASS) {
        return 1;
    }
    vTaskStartScheduler();
    return 1;
}
