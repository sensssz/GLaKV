#!/usr/bin/env bash

export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/tmux/lib:/home/jiamin/usr/lib:/home/jiamin/mysql/lib:$LD_LIBRARY_PATH

salat3=salat3.eecs.umich.edu
db_path=/home/jiamin/speculative/GLaKV/src
output_path=/home/jiamin/speculative/out

num_exp=2000
exp_name="diff_t_m_c_p"

if [[ $# -eq 1 ]]; then
    num_exp=$1
fi

trap 'quit=1' INT

ssh salat3 "mkdir -p ${output_path}/"
ssh salat3 "rm ${output_path}/${exp_name} && touch ${output_path}/${exp_name}"

for t in 10 20 30;
do
    for m in 0.15 0.2 0.25;
    do
        for((c=64;c<=256;c*=2));
        do
            for p in `seq 0 5`;
            do
                if [[ ! -z ${quit+x} ]]; then
                    exit 0
                fi
                ssh salat3 /bin/zsh << EOF
                export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/usr/lib:$LD_LIBRARY_PATH
                echo -n "${t},${m},${c},${p}," >> ${output_path}/${exp_name}
                ${db_path}/glakv_server --dir ${db_path}/glakv_home -p ${p} -n 1 -t ${t} >> ${output_path}/${exp_name}&
EOF
                sleep 2
                ${db_path}/glakv_client -e -m ${m} -c ${c} -n ${num_exp}
                if [[ $? -ne 0 ]]; then
                    ssh salat3 "echo '' >> ${output_path}/${exp_name}"
                fi
                sleep 1
            done
        done
    done
done
