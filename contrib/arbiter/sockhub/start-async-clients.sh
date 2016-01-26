n_clients=8
n_iters=10000
pkill -9 sockub
pkill -9 test-async-client
./sockhub -h $1 -p 5001 -f /tmp/p5002 &
for ((i=0;i<n_clients;i++))
do
    ./test-async-client -h localhost -p 5002 -i $n_iters &
done
