#!/usr/bin/python3

import time
import wtfexpect

GREEN = '\033[92m'
RED = '\033[91m'
NOCOLOR = '\033[0m'

try:
	with wtfexpect.WtfExpect() as we:
		servers = 'alpha bravo conan'.split()
		clients = 'xenon yeast zebra'.split()
		serverids = {name: i for i, name in enumerate(servers)}

		cfg = []
		baseport = 6000
		for i in range(len(servers)):
			cfg.append('-r')
			cfg.append("%d:%s:%d" % (i, "127.0.0.1", baseport + i))

		for i, s in enumerate(servers):
			we.spawn(s, 'bin/server',
				'-i', str(i),
				*cfg,
			)

		for c in clients:
			time.sleep(0.333)
			we.spawn(c, 'bin/client', '-k', c, *cfg)

		start = time.time()
		while we.alive():
			if time.time() - start > 5 and we.alive('alpha'):
				we.kill('alpha')
			timeout = 0.5
			name, line = we.readline(timeout)
			if name is None: continue

			if name in servers:
				src = "%d(%s)" % (serverids[name], name)
			else:
				src = name

			if line is None:
				code = we.getcode(name)
				if code == 0:
					color = GREEN
				else:
					color = RED
				print("%s%s finished with code %d%s" % (color, src, code, NOCOLOR))
			else:
				print("[%s] %s" % (src, line))
except KeyboardInterrupt:
	print("killed")
