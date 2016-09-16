#!/usr/bin/env bash

export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/tmux/lib:/home/jiamin/usr/lib:/home/jiamin/mysql/lib:$LD_LIBRARY_PATH

salat3=salat3.eecs.umich.edu
db_path=/home/jiamin/speculative/GLaKV/src
output_path=/home/jiamin/speculative/out

num_exp=200
exp_name="diff_t_m_c_p_t.pre"

if [[ $# -eq 1 ]]; then
    num_exp=$1
fi

trap 'quit=1' INT

ssh salat3 "mkdir -p ${output_path}/"
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

default_t=10
default_m=0.01
default_c=4
default_think=1000

exp_name="think"
ssh salat3 "rm -f ${output_path}/${exp_name} && touch ${output_path}/${exp_name}"
rm -f ${output_path}/${exp_name} && touch ${output_path}/${exp_name}
t=${default_t}
m=${default_m}
c=${default_c}
think=${default_think}
for((think=300;think<=1300;think+=200));
do
    for p in `seq 0 1`;
    do
        if [[ ! -z ${quit+x} ]]; then
            exit 0
        fi
        ssh salat3 /bin/zsh << EOF
        export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/usr/lib:$LD_LIBRARY_PATH
        echo -n "${think},${p}," >> ${output_path}/${exp_name}
        ${db_path}/glakv_server --dir ${db_path}/glakv_home -p ${p} -n 1 -t ${t} >> ${output_path}/${exp_name}&
EOF
        sleep 2
        echo -n "${think},${p}," >> ${output_path}/${exp_name}
        ${db_path}/glakv_client -e -m ${m} -c ${c} -n ${num_exp} -t ${think} >> ${output_path}/${exp_name}
        if [[ $? -ne 0 ]]; then
            ssh salat3 "echo '' >> ${output_path}/${exp_name}"
        fi
        sleep 1
    done
done

exp_name="prediction"
ssh salat3 "rm -f ${output_path}/${exp_name} && touch ${output_path}/${exp_name}"
rm -f ${output_path}/${exp_name} && touch ${output_path}/${exp_name}
t=${default_t}
m=${default_m}
c=${default_c}
think=${default_think}
for m in 0.01 0.1 0.15 0.2 0.25 0.3;
do
    for p in `seq 0 1`;
    do
        if [[ ! -z ${quit+x} ]]; then
            exit 0
        fi
        ssh salat3 /bin/zsh << EOF
        export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/usr/lib:$LD_LIBRARY_PATH
        echo -n "${m},${p}," >> ${output_path}/${exp_name}
        ${db_path}/glakv_server --dir ${db_path}/glakv_home -p ${p} -n 1 -t ${t} >> ${output_path}/${exp_name}&
EOF
        sleep 2
        echo -n "${m},${p}," >> ${output_path}/${exp_name}
        ${db_path}/glakv_client -e -m ${m} -c ${c} -n ${num_exp} -t ${think} >> ${output_path}/${exp_name}
        if [[ $? -ne 0 ]]; then
            ssh salat3 "echo '' >> ${output_path}/${exp_name}"
        fi
        sleep 1
    done
done

exp_name="prefetch"
ssh salat3 "rm -f ${output_path}/${exp_name} && touch ${output_path}/${exp_name}"
rm -f ${output_path}/${exp_name} && touch ${output_path}/${exp_name}
t=${default_t}
m=${default_m}
c=${default_c}
think=${default_think}
for p in `seq 0 5`;
do
    if [[ ! -z ${quit+x} ]]; then
        exit 0
    fi
    ssh salat3 /bin/zsh << EOF
    export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/usr/lib:$LD_LIBRARY_PATH
    echo -n "{p}," >> ${output_path}/${exp_name}
    ${db_path}/glakv_server --dir ${db_path}/glakv_home -p ${p} -n 1 -t ${t} >> ${output_path}/${exp_name}&
EOF
    sleep 2
    echo -n "${p}," >> ${output_path}/${exp_name}
    ${db_path}/glakv_client -e -m ${m} -c ${c} -n ${num_exp} -t ${think} >> ${output_path}/${exp_name}
    if [[ $? -ne 0 ]]; then
        ssh salat3 "echo '' >> ${output_path}/${exp_name}"
    fi
    sleep 1
done

exp_name="workers"
ssh salat3 "rm -f ${output_path}/${exp_name} && touch ${output_path}/${exp_name}"
rm -f ${output_path}/${exp_name} && touch ${output_path}/${exp_name}
t=${default_t}
m=${default_m}
c=${default_c}
think=${default_think}
for t in 5 10 20 30 40;
do
    for p in `seq 0 1`;
    do
        if [[ ! -z ${quit+x} ]]; then
            exit 0
        fi
        ssh salat3 /bin/zsh << EOF
        export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/usr/lib:$LD_LIBRARY_PATH
        echo -n "${t},${p}," >> ${output_path}/${exp_name}
        ${db_path}/glakv_server --dir ${db_path}/glakv_home -p ${p} -n 1 -t ${t} >> ${output_path}/${exp_name}&
EOF
        sleep 2
        echo -n "${t},${p}," >> ${output_path}/${exp_name}
        ${db_path}/glakv_client -e -m ${m} -c ${c} -n ${num_exp} -t ${think} >> ${output_path}/${exp_name}
        if [[ $? -ne 0 ]]; then
            ssh salat3 "echo '' >> ${output_path}/${exp_name}"
        fi
        sleep 1
    done
done

exp_name="clients"
ssh salat3 "rm -f ${output_path}/${exp_name} && touch ${output_path}/${exp_name}"
rm -f ${output_path}/${exp_name} && touch ${output_path}/${exp_name}
t=${default_t}
m=${default_m}
c=${default_c}
think=${default_think}
for c in 2 4 6 8 10;
do
    for p in `seq 0 1`;
    do
        if [[ ! -z ${quit+x} ]]; then
            exit 0
        fi
        ssh salat3 /bin/zsh << EOF
        export LD_LIBRARY_PATH=/home/jiamin/gcc/lib64:/home/jiamin/usr/lib:$LD_LIBRARY_PATH
        echo -n "${c},${p}," >> ${output_path}/${exp_name}
        ${db_path}/glakv_server --dir ${db_path}/glakv_home -p ${p} -n 1 -t ${t} >> ${output_path}/${exp_name}&
EOF
        sleep 2
        echo -n "${c},${p}," >> ${output_path}/${exp_name}
        ${db_path}/glakv_client -e -m ${m} -c ${c} -n ${num_exp} -t ${think} >> ${output_path}/${exp_name}
        if [[ $? -ne 0 ]]; then
            ssh salat3 "echo '' >> ${output_path}/${exp_name}"
        fi
        sleep 1
    done
done