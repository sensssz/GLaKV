#!/usr/bin/env bash

export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/tmux/lib:/home/jiamin/usr/lib:/home/jiamin/mysql/lib:$LD_LIBRARY_PATH

salat3=salat3.eecs.umich.edu
db_path=/home/jiamin/speculative/GLaKV/src
output_path=/home/jiamin/speculative/out

num_exp=10000
exp_name="diff_c_p"

if [[ $# -eq 1 ]]; then
    num_exp=$1
fi

trap 'quit=1' INT

ssh salat3 "mkdir -p ${output_path}/"
ssh salat3 "rm ${output_path}/${exp_name} && touch ${output_path}/${exp_name}"

for p in `seq 0 5`;
do
    for((c=1;c<=128;c*=2))
    do
        if [[ ! -z ${quit+x} ]]; then
            exit 0
        fi
        ssh salat3 /bin/zsh << EOF
        export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/usr/lib:$LD_LIBRARY_PATH
        echo -n "${p},${c}," >> ${output_path}/${exp_name}
        ${db_path}/glakv_server --dir ${db_path}/glakv_home -p ${p} -n 1 >> ${output_path}/${exp_name}&
EOF
        sleep 2
        ${db_path}/glakv_client -e -c ${c} -n ${num_exp}
        sleep 1
    done
done
