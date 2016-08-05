#!/usr/bin/python3

import select
import subprocess
import time

class Proc():
	def __init__(self, name, *argv):
		self.p = subprocess.Popen(
			argv, bufsize=0,
			stdin=subprocess.PIPE,
			stdout=subprocess.PIPE,
			stderr=subprocess.STDOUT,
		)
		self.name = name
		self.accum = bytes()

	def fileno(self):
		return self.p.stdout.fileno()

	def readlines(self):
		newbytes = self.p.stdout.read(1024)
		if len(newbytes):
			self.accum += newbytes
			*newlines, self.accum = self.accum.split(b'\n')
			return [l.decode() for l in newlines]
		else:
			self.p.stdout.close()
			if len(self.accum):
				return [self.accum.decode(), None]
			else:
				return [None]

	def eof(self):
		return self.p.stdout.closed

	def kill(self):
		self.p.kill()

	def wait(self):
		return self.p.wait()

class WtfExpect():
	def __init__(self):
		self.procs = {}
		self.retcodes = {}
		self.lines = []

	def __enter__(self):
		return self

	def __exit__(self, *exc):
		self.finish()
		return False

	def run(self, argv):
		p = subprocess.run(
			argv,
			stdout=subprocess.PIPE,
			stderr=subprocess.STDOUT,
		)
		return p.returncode, p.stdout

	def spawn(self, name, *argv):
		assert(name not in self.procs)
		self.procs[name] = Proc(name, *argv)
		return True

	def kill(self, name):
		assert(name in self.procs)
		self.procs[name].kill()

	def close(self, name):
		assert(name in self.procs)
		self.retcodes[name] = self.procs[name].wait()
		del self.procs[name]

	def readline(self, timeout=None):
		if len(self.lines):
			return self.lines.pop(0)

		active = [p for p in self.procs.values() if not p.eof()]
		ready, _, _ = select.select(active, [], [], timeout)
		if len(ready):
			for proc in ready:
				for line in proc.readlines():
					self.lines.append((proc.name, line))
					if line is None:
						self.kill(proc.name)
						self.close(proc.name)
			if len(self.lines):
				return self.lines.pop(0)

		return None, None

	def expect(self, patterns, timeout=None):
		started = time.time()
		while self.alive():
			if timeout is not None:
				t = time.time()
				if t - started > timeout:
					return None, None
				elapsed = t - started
				timeleft = timeout - elapsed
			else:
				timeleft = None

			name, line = self.readline(timeleft)
			if line is None:
				return name, None
			if name not in patterns:
				continue
			if line in patterns[name]:
				return name, line

	def capture(self, *names):
		results = {}
		nameslist = list(names)
		for name in names:
			assert(name in self.names.values())
			results[name] = {
				'retcode': None,
				'output': [],
			}
		while len(nameslist):
			aname, line = self.readline()
			if aname not in nameslist:
				continue
			if line is None:
				results[aname]['retcode'] = self.getcode(aname)
				nameslist.remove(aname)
			else:
				results[aname]['output'].append(line)
		return results

	def getcode(self, name):
		if name in self.retcodes:
			retcode = self.retcodes[name]
			del self.retcodes[name]
			return retcode
		return None

	def alive(self, name=None):
		if name is not None:
			return name in self.procs
		else:
			return len(self.procs) > 0

	def finish(self):
		for proc in self.procs.values():
			proc.kill()
		self.procs = {}
		self.retcodes = {}
		self.lines = []
