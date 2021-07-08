#!/usr/bin/python
import json
import sys
import os
import time
import subprocess
from sys import argv,exit

timeout = 60

sys.path.append(os.path.join(os.path.dirname(__file__), '../qemu-4.2.0/python'))

from qemu import qmp

def main():
    timeout_start = time.time()
    qemu = qmp.QEMUMonitorProtocol("../qmp-vinput-sock")
    while True:
        try:
            qemu.connect(negotiate=True)
            break
        except qmp.QMPConnectError:
            print('Didn\'t get QMP greeting message from QEMU QMP server')
        except qmp.QMPCapabilitiesError:
            print('Could not negotiate capabilities with QEMU QMP server')
        except qemu.error:
            print

        if time.time() >= timeout + timeout_start:
            print("connection timeout error")
            return
    resp = qemu.cmd('query-status')
    if resp != None:
        for val in resp.values():
            if val['status'] == "suspended":
                qemu.cmd('system_wakeup')


if __name__ == '__main__':
    main()
