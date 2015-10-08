n_clients=100
n_iters=100000
./sockhub -h $1 -p 5001 -f /tmp/p5002 &
for ((i=0;i<n_clients;i++))
do
    ./test-client localhost 5002 $n_iters &
done
