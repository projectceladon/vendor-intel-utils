#!/usr/bin/python2.7

import os
import sys
import urllib2
import ConfigParser
from sgmllib import SGMLParser

class GetLstVersion(SGMLParser):

    is_a = ""
    version = []

    def start_a(self, attrs):
        self.is_a = "1"

    def end_a(self):
        self.is_a = ""

    def handle_data(self, text):
        if self.is_a:
            self.version.append(text)

def main():

    CONFIG_FILE = sys.argv[1]
    sos_date_version = sys.argv[2]

    if os.path.exists(os.path.join(os.getcwd(), CONFIG_FILE)):
        config = ConfigParser.ConfigParser()
        config.read(CONFIG_FILE)

    sos_link = config.get("LINK", "SOS_LINK")
    sos_default_version = config.get("DEFAULT", "SOS_VERSION")

    if not sos_date_version.strip():
        sos_date_version = sos_default_version

    if sos_date_version == "latest":
        acrnversion = "lst"
        acrnlink = sos_link
        content = urllib2.urlopen(acrnlink).read()
        num = GetLstVersion()
        num.feed(content)
        v = num.version[-5]
        v = v[:-1]
        acrnversion = "sos_version_" + sos_date_version + "_" + v
        acrnlink = acrnlink + "/" + v + "/gordonpeak/virtualization"
    else:
        acrnlink = sos_link + "/" + sos_date_version + "/gordonpeak/virtualization"
        acrnversion = "sos_version" + "_" + sos_date_version

    try:
        urllib2.urlopen(acrnlink)
    except:
        print "wrong link"

    print acrnversion
    print acrnlink

if __name__ == '__main__':
    main()

