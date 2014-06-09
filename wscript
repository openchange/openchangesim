#! /usr/bin/env python
# encoding: utf-8

from waflib import Task
from waflib.TaskGen import extension
from waflib.Tools import bison
from waflib.Tools import flex

top = '.'
out = 'build'

APPNAME = 'openchangesim'
VERSION = 1.0

def options(ctx):
    ctx.load('compiler_c')

def configure(ctx):
    ctx.load('compiler_c')

    ctx.define('_GNU_SOURCE', 1)
    ctx.env.append_value('CCDEFINES', '_GNU_SOURCE=1')

    # Check headers
    ctx.check(header_name='sys/types.h')
    ctx.check(header_name='sys/mman.h')
    ctx.check(header_name='sys/stat.h')
    ctx.check(header_name='sys/ioctl.h')
    ctx.check(header_name='syslog.h')
    ctx.check(header_name='stdio.h')
    ctx.check(header_name='stdlib.h')
    ctx.check(header_name='stdbool.h')
    ctx.check(header_name='stdarg.h')
    ctx.check(header_name='unistd.h')
    ctx.check(header_name='stdarg.h')
    ctx.check(header_name='signal.h')
    ctx.check(header_name='net/if.h')
    ctx.check(header_name='linux/if_tun.h')

    # Check types
    ctx.check(type_name='uint8_t')
    ctx.check(type_name='uint16_t')
    ctx.check(type_name='uint32_t')
    ctx.check(type_name='uint64_t')
    ctx.check(type_name='bool', header_name='stdbool.h')

    # Check functions
    ctx.check_cc(function_name='memcpy', header_name='string.h', mandatory=True)
    ctx.check_cc(function_name='memset', header_name='string.h', mandatory=True)
    ctx.check_cc(function_name='openlog', header_name='syslog.h', mandatory=True)
    ctx.check_cc(function_name='syslog', header_name='syslog.h', mandatory=True)
    ctx.check_cc(function_name='closelog', header_name='syslog.h', mandatory=True)

    ctx.find_program('bison', var='BISON')
    ctx.env.BISONFLAGS = ['-d']

    ctx.find_program('flex', var='FLEX')
    ctx.env.FLEXFLAGS = ['-t']

    # Check external libraries and packages
    ctx.check_cfg(atleast_pkgconfig_version='0.20')
    ctx.check_cfg(package='talloc',
                  args=['talloc >= 2.0.7', '--cflags', '--libs'],
                  uselib_store='TALLOC',
                  msg="Checking for talloc 2.0.7",
                  mandatory=True)

    ctx.check_cfg(package='popt',
                  args=['popt >= 1.16', '--cflags', '--libs'],
                  uselib_store='POPT',
                  msg="Checking for libpopt 1.16",
                  mandatory=True)

    ctx.check_cfg(package='libmapi',
                  args=['libmapi >= 2.0', '--cflags', '--libs'],
                  uselib_store='LIBMAPI',
                  msg="Checking for libmapi 2.0",
                  mandatory=True)

    ctx.write_config_header('config.h')

def pre(ctx):
    ctx.exec_command('rm -f src/version.h;./script/mkversion.sh VERSION %s/version.h' % out)

def build(ctx):
    ctx.add_pre_fun(pre)
    ctx.program(
        source = [
            'src/configuration.l',
            'src/configuration.y',
            'src/configuration_api.c',
            'src/configuration_dump.c',
            'src/openchangesim_public.c',
            'src/openchangesim_interface.c',
            'src/openchangesim_modules.c',
            'src/openchangesim_fork.c',
            'src/openchangesim_logs.c',
            'src/openchangesim.c',
            'src/modules/module_fetchmail.c',
            'src/modules/module_sendmail.c',
            'src/modules/module_cleanup.c'
        ],
        includes = ['src', '.', 'build'],
        target = 'openchangesim',
        use = ['TALLOC', 'LIBMAPI', 'POPT'])
