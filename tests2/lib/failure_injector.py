import docker

class FailureInjector(object):

    # def __init__(self):
    #     self.docker_api = docker.Client()

    def container_exec(self, node, command):
        self.docker_api = docker.Client()
        exec_id = self.docker_api.exec_create(node, command, user='root')
        output = self.docker_api.exec_start(exec_id)
        self.docker_api.close()
        # print(command, ' -> ', output)


class SingleNodePartition(FailureInjector):

    def __init__(self, node):
        self.node = node
        super().__init__()

    def start(self):
        # XXX: try reject too
        self.container_exec(self.node, "iptables -A INPUT -j DROP")
        self.container_exec(self.node, "iptables -A OUTPUT -j DROP")

    def stop(self):
        self.container_exec(self.node, "iptables -D INPUT -j DROP")
        self.container_exec(self.node, "iptables -D OUTPUT -j DROP")


class EdgePartition(FailureInjector):

    def __init__(self, nodeA, nodeB):
        self.nodeA = nodeA
        self.nodeB = nodeB
        super().__init__()

    def start(self):
        self.container_exec(self.nodeA, "iptables -A INPUT -s {} -j DROP".format(self.nodeB) )
        self.container_exec(self.nodeA, "iptables -A OUTPUT -s {} -j DROP".format(self.nodeB) )

    def stop(self):
        self.container_exec(self.nodeA, "iptables -D INPUT -s {} -j DROP".format(self.nodeB))
        self.container_exec(self.nodeA, "iptables -D OUTPUT -s {} -j DROP".format(self.nodeB))


class RestartNode(FailureInjector):

    def __init__(self, node):
        self.node = node
        super().__init__()

    # XXX: Is it really a good idea to call cli.stop inside method called start?
    def start(self):
        self.docker_api.stop(self.node)

    def stop(self):
        self.docker_api.start(self.node)


class CrashRecoverNode(FailureInjector):

    def __init__(self, node):
        self.node = node
        super().__init__()

    def start(self):
        self.docker_api.kill(self.node)

    def stop(self):
        self.docker_api.start(self.node)


class SkewTime(FailureInjector):

    def __init__(self, node):
        self.node = node
        super().__init__()
