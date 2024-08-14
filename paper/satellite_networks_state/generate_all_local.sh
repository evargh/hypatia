num_threads=20
for i in 5 8 11 4 7 10
do
  bash generate_for_paper.sh ${i} ${num_threads} || exit 1
done
