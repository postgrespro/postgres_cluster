ansible-playbook -i deploy/hosts deploy/cluster.yml
ansible-playbook -i deploy/hosts perf.yml -e nnodes=2
ansible-playbook -i deploy/hosts deploy/cluster.yml
ansible-playbook -i deploy/hosts perf.yml -e nnodes=3
ansible-playbook -i deploy/hosts deploy/cluster.yml
ansible-playbook -i deploy/hosts perf.yml -e nnodes=4
ansible-playbook -i deploy/hosts deploy/cluster.yml
ansible-playbook -i deploy/hosts perf.yml -e nnodes=5

