cmd_/home/kpirap18/sem6/BMSTU-OS-sem6/lab_03/part_02/modules.order := {   echo /home/kpirap18/sem6/BMSTU-OS-sem6/lab_03/part_02/md1.ko;   echo /home/kpirap18/sem6/BMSTU-OS-sem6/lab_03/part_02/md2.ko;   echo /home/kpirap18/sem6/BMSTU-OS-sem6/lab_03/part_02/md3.ko; :; } | awk '!x[$$0]++' - > /home/kpirap18/sem6/BMSTU-OS-sem6/lab_03/part_02/modules.order