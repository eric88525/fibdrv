#!/bin/bash
for file in `find /proc/irq -name "smp_affinity"`
do
    sudo bash -c "echo 7fff > ${file}"
done
sudo bash -c "echo 7fff > /proc/irq/default_smp_affinity"