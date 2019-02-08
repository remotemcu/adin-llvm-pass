import os
import sys
import re

RUN_TOKEN = "; RUN: "
REPLACE_TOKEN = "%s"

if __name__=='__main__':
	RUN_LINE = ""
	NAME_FILE = sys.argv[1]
	with open(NAME_FILE, 'r') as f:
		while True:
			line = f.readline()
			raw = line.splitlines()[0]
			is_run = re.match("^" + RUN_TOKEN, raw)
			if is_run:
				if raw.endswith('\\'):
					raw = raw[:-1]
				raw = raw.split(RUN_TOKEN)[1]
				RUN_LINE += raw
			else:
				break
		COMMAND = RUN_LINE.replace(REPLACE_TOKEN, NAME_FILE)
		code = os.system(COMMAND)
		if code is not 0:
			sys.exit(1)
