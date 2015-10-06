n_clients=10
n_iters=100000
for ((i=0;i<n_clients;i++))
do
    ./test-client $1 5001 $n_iters &
done
wait
