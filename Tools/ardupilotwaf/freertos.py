# encoding: utf-8

"""
Generic Waf tool for AP_HAL_FreeRTOS.
"""

import importlib.util
from waflib.Configure import conf
import os
import sys

SUPPORTED_PLATFORMS = (
    'rp2350',      # AP_HAL_FreeRTOS/platforms/rp2350/waf.py
    #'esp32',       # AP_HAL_FreeRTOS/platforms/esp32/waf.py
    #'imxrt',       # AP_HAL_FreeRTOS/platforms/imxrt/waf.py
    #'renesas_ra',  # AP_HAL_FreeRTOS/platforms/renesas_ra/waf.py
    #'ti_am26x',    # AP_HAL_FreeRTOS/platforms/ti_am26x/waf.py
)

class freertos_platform:
    def __init__(self, ctx):
        self.ctx = ctx
        self.platform = self._get_name()
        self.module = None
        self.module = self._load_module()

    def _get_name(self):
        platform = self.ctx.env.FREERTOS_PLATFORM
        if not platform:
            self.ctx.fatal("FREERTOS_PLATFORM is not defined")

        platform = str(platform).lower()
        if platform not in SUPPORTED_PLATFORMS:
            self.ctx.fatal(
                "Unsupported FreeRTOS platform: %s. Supported platforms: %s"
                % (
                    platform,
                    ', '.join(SUPPORTED_PLATFORMS),
                )
            )
        return platform

    def _get_waf_path(self):
        return self.ctx.srcnode.make_node(
            'libraries/AP_HAL_FreeRTOS/platforms/%s/waf.py'
            % self.platform
        ).abspath()

    def _load_module(self):
        if self.module is not None:
            return self.module

        waf_path = self._get_waf_path()
        if not os.path.isfile(waf_path):
            self.ctx.fatal(
                "Platform Waf file not found: %s"
                % waf_path
            )

        module_name = 'ap_hal_freertos_%s_waf' % self.platform
        if module_name in sys.modules:
            self.module = sys.modules[module_name]
            return self.module

        spec = importlib.util.spec_from_file_location(
            module_name,
            waf_path
        )
        if spec is None or spec.loader is None:
            self.ctx.fatal(
                "Failed to create module spec for: %s"
                % waf_path
            )

        module = importlib.util.module_from_spec(spec)
        sys.modules[module_name] = module

        try:
            spec.loader.exec_module(module)
        except Exception as ex:
            self.ctx.fatal(
                "Failed to load FreeRTOS platform %s:\n%s"
                % (self.platform, ex)
            )
        self.module = module
        return self.module

    def configure_env(self, env):
        if hasattr(self.module, 'configure_env'):
            self.module.configure_env(self.ctx, env)

    def configure(self):
        self.module.configure(self.ctx)

    def pre_build(self):
        if hasattr(self.module, 'pre_build'):
            self.module.pre_build(self.ctx)

@conf
def freertos_configure_env(cfg, env):
    freertos_platform(cfg).configure_env(env)

def configure(cfg):
    freertos_platform(cfg).configure()

def pre_build(bld):
    freertos_platform(bld).pre_build()