import docker

class FailureInjector(object):

    def __init__(self):
        self.docker_api = docker.from_env()

    def container_exec(self, node, command):
        docker_node = self.docker_api.containers.get(node)
        docker_node.exec_run(command, user='root')

class NoFailure(FailureInjector):

    def start(self):
        return

    def stop(self):
        return

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
        self.docker_api.containers.get(self.node).stop()

    def stop(self):
        self.docker_api.containers.get(self.node).start()


class CrashRecoverNode(FailureInjector):

    def __init__(self, node):
        self.node = node
        super().__init__()

    def start(self):
        self.docker_api.containers.get(self.node).kill()

    def stop(self):
        self.docker_api.containers.get(self.node).start()


class SkewTime(FailureInjector):

    def __init__(self, node):
        self.node = node
        super().__init__()

class StopNode(FailureInjector):

    def __init__(self, node):
        self.node = node
        super().__init__()

    # XXX: Is it really a good idea to call cli.stop inside method called start?
    def start(self):
        self.docker_api.containers.get(self.node).stop()

    def stop(self):
        return


class StartNode(FailureInjector):

    def __init__(self, node):
        self.node = node
        super().__init__()

    # XXX: Is it really a good idea to call cli.stop inside method called start?
    def start(self):
        return

    def stop(self):
        self.docker_api.containers.get(self.node).start()
