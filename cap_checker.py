#!/usr/bin/env python
'''
This tool tests that the following AREQs are not violated:
https://jira01.devtools.intel.com/browse/AREQ-22708 (No setuid/setgid)
https://jira01.devtools.intel.com/browse/AREQ-21996 (No dangerous caps)
'''

import argparse
if sys.version_info < (3, 0, 1):
    import ConfigParser as configparser
else:
    import configparser as configparser #Python 3 renamed this
import os
import re
import subprocess
import sys


class CapabiltyChecker():
    '''Checks an sepolicy file for capability violations'''

    def __init__(self, config_file):
        '''Initializes a CapabiltyChecker'''

        self._config_file = config_file
        if sys.version_info < (3, 0, 1):
            self._config = configparser.ConfigParser()
        else:
            self._config = configparser.ConfigParser(strict=False)

        self._dissallowed_caps = None
        self._aosp_ignored = None
        self._device_ignored = None
        self._device_bug = None

        self.parse()

    @property
    def dissallowed_caps(self):
        '''Set of capabilities not allowed'''

        return self._dissallowed_caps

    @property
    def aosp_ignored(self):
        '''Known capabilities in the AOSP base policy'''

        return self._aosp_ignored

    @property
    def device_ignored(self):
        '''Known, but OK, uses of dissallowed_caps in device policy'''

        return self._device_ignored

    @property
    def device_bug(self):
        '''Capability usages that violate policy but have known filed bugs'''

        return self._device_bug

    def parse(self):
        '''Reads the config file and initializes internal data structures'''

        config = self._config

        config.read(self._config_file)
        dissallowed_caps = set(config.get('DISSALLOWED_CAPS', 'caps').split()) \
            if config.has_section('DISSALLOWED_CAPS') else set()

        aosp_ignored = {}
        if config.has_section('AOSP_IGNORED'):
            for domain, caplist in config.items('AOSP_IGNORED'):
                aosp_ignored[domain] = set(caplist.split())

        device_ignored = {}
        if config.has_section('DEVICE_IGNORED'):
            for domain, caplist in config.items('DEVICE_IGNORED'):
                capset = dissallowed_caps if caplist == '*' else set(
                    caplist.split())
                device_ignored[domain] = capset

        # dict of tuple(set, bugstring)
        device_bug = {}
        if config.has_section('DEVICE_BUG'):
            for domain, caplist in config.items('DEVICE_BUG'):
                capset = set()
                bug = None

                for item in caplist.split():
                    if item.startswith("http"):
                        bug = item
                    else:
                        capset.add(item)

                device_bug[domain] = (capset, bug)

        self._dissallowed_caps = dissallowed_caps
        self._aosp_ignored = aosp_ignored
        self._device_ignored = device_ignored
        self._device_bug = device_bug

    def search(self, sepolicy, include_bug=False):
        '''Searches an sepolicy for policy violations'''

        cmd = 'sesearch -A -c capability,capability2 %s' % sepolicy

        pipe = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)

        stdout, stderr = pipe.communicate()

        if pipe.returncode != 0:
            sys.exit(
                'Could not execute sesearch, return code: %d' % pipe.returncode)

        found = {}

        for allow in stdout.split('\n'):
            allow = allow.strip()
            if len(allow) == 0:
                continue

            allow = allow.rstrip(';')

            source = allow.split()[1]

            source = CapabiltyChecker.strip_version(source)

            aosp_caps = self.aosp_ignored[
                source] if source in self.aosp_ignored else set()

            if '{' in allow:
                cap_start = allow.index('{')
                cap_end = allow.index('}')
                caps = set(allow[cap_start + 1:cap_end - 1].split())
            else:
                caps = set([allow.split()[-1]])

            # We only care if we added it to an AOSP bug_domain and it violates the AREQ
            caps -= aosp_caps
            caps &= self.dissallowed_caps

            if len(caps) == 0:
                continue

            bug_report = None
            violation_caps = caps

            if source in self.device_bug:
                (bug_caps, bug_ticket) = self.device_bug[source]
                violation_caps -= bug_caps

                if include_bug:
                    bug_report = {'bug': (bug_caps, bug_ticket)}

            if source in self.device_ignored:
                ignored_caps = self.device_ignored[source]

                violation_caps = caps - ignored_caps

            if len(violation_caps) > 0:
                violator_report = {
                    'violators': (violation_caps, 'No Bug Filed!')
                }
                if bug_report:
                    violator_report.update(bug_report)
                found[source] = violator_report

        return found

    #pylint: disable=redefined-outer-name
    @staticmethod
    def strip_version(source):
        '''Strips the version suffix from a domain string.
           For example, foo_current or foo_100 becomes foo.
        '''

        # strip version suffix
        if source.endswith('_current'):
            source = source[:-len('_current')]
        source = re.split(r'_[0-9]+_[0-9]+', source)[0]
        return source


def handle_options():
    '''Parse the command line arguments and return the arguments'''

    parser = argparse.ArgumentParser(description=(
        'This tool searches a binary sepolicy file for AREQ violations against:\n'
        'https://jira01.devtools.intel.com/browse/AREQ-22708 (No setuid/setgid)\n'
        'https://jira01.devtools.intel.com/browse/AREQ-21996 (No dangerous caps)\n\n'
        'It exits to the shell the number of violations found, thus 0 means success.'
    ))

    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        '-s', '--show-bug', help='Show current known bugs', action='store_true')
    group.add_argument(
        '-i',
        '--include-bug',
        help=
        ('include the known bugs in the report output, ie shows unfixed things as well as new'
         'things'),
        action='store_true')
    group.add_argument(
        '-t',
        '--train',
        help=(
            'Train against AOSP/Base policy. Thus, the sepolicy file must be a '
            'base sepolicy file, it outputs a fata structure for AOSP_IGNORE.'),
        action='store_true')

    parser.add_argument(
        '-p',
        '--policy',
        help='A config file of known policy capability usages.',)

    parser.add_argument(
        'sepolicy',
        nargs='?',
        default=os.path.join(os.environ['OUT'], 'root', 'sepolicy'),
        help=
        'The sepolicy file to search for violations, the default is $OUT/root/sepolicy'
    )

    return parser.parse_args()


def main():
    '''Main Program Entry Point'''

    args = handle_options()

    cap_checker = CapabiltyChecker(args.policy)

    if args.show_bug:
        print('Current known bugs: %s' % cap_checker.device_bug)
        sys.exit(0)

    found = cap_checker.search(args.sepolicy, args.include_bug)

    found_len = len(found)

    if found_len > 0:
        if not args.train:
            print(
                'Please look at patching device/intel/sepolicy/tools/cap_checker.py\n'
                'in either device_bug or device_ignored based on the output below.\n'
                'This tool serves as a build time validation of AREQs:\n'
                '  * https://jira01.devtools.intel.com/browse/AREQ-21996\n'
                '  * https://jira01.devtools.intel.com/browse/AREQ-22708 (partial)\n'
                'NOTE: These AREQ links may be to different devices, but they apply to\n'
                '      to the codebase as a whole.\n'
                'contact William Roberts <william.c.roberts@intel.com> for help.\n'
                '\n\nSecurity Violations:')

        for domain, report in found.items():
            if args.train:
                print(("'%s' : %s" % (domain, ' '.join(report['violators'][0]))))
            else:
                msg = ("Domain: '%s'\n"
                       "Violations:\n"
                       "  New: '%s'") % (domain,
                                         ', '.join(report['violators'][0]))
                if 'bug' in report:
                    msg += "\n  Old: '%s' : See Bug(s): '%s'" % (
                        ', '.join(report['bug'][0]), report['bug'][1])
                print(msg)

    sys.exit(found_len)


if __name__ == "__main__":
    main()
