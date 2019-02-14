# Copyright (c) 2018 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

import abc
import argparse
import os
import subprocess

from west import cmake
from west import log
from west.build import is_zephyr_build
from west.util import quote_sh_list

from runners.core import BuildConfiguration

from zephyr_ext_common import find_build_dir, Forceable, \
    BUILD_DIR_DESCRIPTION, cached_runner_config

SIGN_DESCRIPTION = '''\
This command automates some of the drudgery of creating signed Zephyr
binaries for chain-loading by a bootloader.

In the simplest usage, run this from your build directory:

   west sign -t your_tool -- ARGS_FOR_YOUR_TOOL

Assuming your binary was properly built for processing and handling by
tool "your_tool", this creates zephyr.signed.bin and zephyr.signed.hex
files (if supported by "your_tool") which are ready for use by your
bootloader. The "ARGS_FOR_YOUR_TOOL" value can be any additional
arguments you want to pass to the tool, such as the location of a
signing key, a version identifier, etc.

See tool-specific help below for details.'''

SIGN_EPILOG = '''\
imgtool
-------

Currently, MCUboot's 'imgtool' tool is supported. To build a signed
binary you can load with MCUboot using imgtool, run this from your
build directory:

   west sign -t imgtool -- --key YOUR_SIGNING_KEY.pem

The image header size, alignment, and slot sizes are determined from
the build directory using board information and the device tree. A
default version number of 0.0.0+0 is used (which can be overridden by
passing "--version x.y.z+w" after "--key"). As shown above, extra
arguments after a '--' are passed to imgtool directly.'''


class ToggleAction(argparse.Action):

    def __call__(self, parser, args, ignored, option):
        setattr(args, self.dest, not option.startswith('--no-'))


class Sign(Forceable):
    def __init__(self):
        super(Sign, self).__init__(
            'sign',
            # Keep this in sync with the string in west-commands.yml.
            'sign a Zephyr binary for bootloader chain-loading',
            SIGN_DESCRIPTION,
            accepts_unknown_args=False)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            epilog=SIGN_EPILOG,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description)

        parser.add_argument('-d', '--build-dir', help=BUILD_DIR_DESCRIPTION)
        self.add_force_arg(parser)

        # general options
        group = parser.add_argument_group('tool control options')
        group.add_argument('-t', '--tool', choices=['imgtool'],
                           help='image signing tool name')
        group.add_argument('-p', '--tool-path', default='imgtool',
                           help='''path to the tool itself, if needed''')
        group.add_argument('tool_args', nargs='*', metavar='tool_opt',
                           help='extra option(s) to pass to the signing tool')

        # bin file options
        group = parser.add_argument_group('binary (.bin) file options')
        group.add_argument('--bin', '--no-bin', dest='gen_bin', nargs=0,
                           action=ToggleAction,
                           help='''produce a signed .bin file?
                            (default: yes, if supported)''')
        group.add_argument('-B', '--sbin', metavar='BIN',
                           default='zephyr.signed.bin',
                           help='''signed .bin file name
                           (default: zephyr.signed.bin)''')

        # hex file options
        group = parser.add_argument_group('Intel HEX (.hex) file options')
        group.add_argument('--hex', '--no-hex', dest='gen_hex', nargs=0,
                           action=ToggleAction,
                           help='''produce a signed .hex file?
                           (default: yes, if supported)''')
        group.add_argument('-H', '--shex', metavar='HEX',
                           default='zephyr.signed.hex',
                           help='''signed .hex file name
                           (default: zephyr.signed.hex)''')

        # defaults for hex/bin generation
        parser.set_defaults(gen_bin=True, gen_hex=True)

        return parser

    def do_run(self, args, ignored):
        if not (args.gen_bin or args.gen_hex):
            return

        self.check_force(os.path.isdir(args.build_dir),
                         'no such build directory {}'.format(args.build_dir))
        self.check_force(is_zephyr_build(args.build_dir),
                         "build directory {} doesn't look like a Zephyr build "
                         'directory'.format(args.build_dir))

        if args.tool == 'imgtool':
            signer = ImgtoolSigner()
        # (Add support for other signers here in elif blocks)
        else:
            raise RuntimeError("can't happen")

        # Provide the build directory if not given, and defer to the signer.
        args.build_dir = find_build_dir(args.build_dir)
        signer.sign(args)


class Signer(abc.ABC):
    '''Common abstract superclass for signers.

    To add support for a new tool, subclass this and add support for
    it in the Sign.do_run() method.'''

    @abc.abstractmethod
    def sign(self, args):
        '''Abstract method to perform a signature; subclasses must implement.

        :param args: parsed arguments from Sign command
        '''


class ImgtoolSigner(Signer):

    def sign(self, args):
        cache = cmake.CMakeCache.from_build_dir(args.build_dir)
        runner_config = cached_runner_config(args.build_dir, cache)
        bcfg = BuildConfiguration(args.build_dir)

        # Build a signed .bin
        if args.gen_bin and runner_config.bin_file:
            sign_bin = self.sign_cmd(args, bcfg, runner_config.bin_file,
                                     args.sbin)
            log.dbg(quote_sh_list(sign_bin))
            subprocess.check_call(sign_bin)

        # Build a signed .hex
        if args.gen_hex and runner_config.hex_file:
            sign_hex = self.sign_cmd(args, bcfg, runner_config.hex_file,
                                     args.shex)
            log.dbg(quote_sh_list(sign_hex))
            subprocess.check_call(sign_hex)

    def sign_cmd(self, args, bcfg, infile, outfile):
        align = str(bcfg['DT_FLASH_WRITE_BLOCK_SIZE'])
        vtoff = str(bcfg['CONFIG_TEXT_SECTION_OFFSET'])
        slot_size = str(bcfg['DT_FLASH_AREA_IMAGE_0_SIZE'])

        sign_command = [args.tool_path or 'imgtool',
                        'sign',
                        '--align', align,
                        '--header-size', vtoff,
                        '--slot-size', slot_size,
                        # We provide a default --version in case the
                        # user is just messing around and doesn't want
                        # to set one. It will be overridden if there is
                        # a --version in args.tool_args.
                        '--version', '0.0.0+0',
                        infile,
                        outfile]

        sign_command.extend(args.tool_args)

        return sign_command
