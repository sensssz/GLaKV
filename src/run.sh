#!/usr/bin/env bash

export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/tmux/lib:/home/jiamin/usr/lib:/home/jiamin/mysql/lib:$LD_LIBRARY_PATH

salat3=salat3.eecs.umich.edu
db_path=/home/jiamin/speculative/GLaKV/src
output_path=/home/jiamin/speculative/out

num_exp=100
exp_name="diff_t_m_c_p_t.pre"

if [[ $# -eq 1 ]]; then
    num_exp=$1
fi

trap 'quit=1' INT

ssh salat3 "mkdir -p ${output_path}/"
ssh salat3 "rm -f ${output_path}/${exp_name} && touch ${output_path}/${exp_name}"
mkdir -p ${output_path}/
#echo 'num_workers,lambda,num_clients,think,num_prefetch,avg_latency,num_exps' >> ${output_path}/${exp_name}

#for t in 10 20;
#do
#    for m in 0.01 0.05 0.1;
#    do
#        for((c=1;c<=8;c*=2));
#        do
#            for((think=500;think<=2000;think+=500));
#            do
#                for p in `seq 0 5`;
#                do
#                    if [[ ! -z ${quit+x} ]]; then
#                        exit 0
#                    fi
#                    ssh salat3 /bin/zsh << EOF
#                    export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/usr/lib:$LD_LIBRARY_PATH
#                    echo -n "${t},${m},${c},${think},${p}," >> ${output_path}/${exp_name}
#                    ${db_path}/glakv_server --dir ${db_path}/glakv_home -p ${p} -n 1 -t ${t} >> ${output_path}/${exp_name}&
#EOF
#                    sleep 2
#                    echo -n "${t},${m},${c},${think},${p}," >> ${output_path}/${exp_name}
#                    ${db_path}/glakv_client -e -m ${m} -c ${c} -n ${num_exp} -t ${think} >> ${output_path}/${exp_name}
#                    if [[ $? -ne 0 ]]; then
#                        ssh salat3 "echo '' >> ${output_path}/${exp_name}"
#                    fi
#                    sleep 1
#                done
#            done
#        done
#    done
#done

exp_name="think"
rm -f ${output_path}/${exp_name} && touch ${output_path}/${exp_name}
t=10
m=10
c=4
p=1
for((think=500;think<=1100;think+=200));
do
    if [[ ! -z ${quit+x} ]]; then
        exit 0
    fi
    ssh salat3 /bin/zsh << EOF
    export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/usr/lib:$LD_LIBRARY_PATH
    echo -n "${think}" >> ${output_path}/${exp_name}
    ${db_path}/glakv_server --dir ${db_path}/glakv_home -p ${p} -n 1 -t ${t} >> ${output_path}/${exp_name}&
EOF
    sleep 2
    echo -n "${think}" >> ${output_path}/${exp_name}
    ${db_path}/glakv_client -e -m ${m} -c ${c} -n ${num_exp} -t ${think} >> ${output_path}/${exp_name}
    if [[ $? -ne 0 ]]; then
        ssh salat3 "echo '' >> ${output_path}/${exp_name}"
    fi
    sleep 1
done

