# test for txtalign file_length: 500
for k in 16 32 48 64
do
    echo "./no_multiple_kmins -src_path ./source_500/ -query_file ./source_500/source-document03260.txt -k $k " >> no_multi_kmins.txt
    ./no_multiple_kmins -src_path ./source_500/ -query_file ./source_500/source-document03260.txt -k $k  >> no_multi_kmins.txt
done

# test for txtalign file_length: 1000
for k in 16 32 48 64
do
    echo "./no_multiple_kmins -src_path ./source_1000/ -query_file ./source_1000/source-document02165.txt -k $k " >> no_multi_kmins.txt
    ./no_multiple_kmins -src_path ./source_1000/ -query_file ./source_1000/source-document02165.txt -k $k  >> no_multi_kmins.txt
done

# test for txtalign file_length: 2000
for k in 16 32 48 64
do
    echo "./no_multiple_kmins -src_path ./source_2000/ -query_file ./source_2000/source-document01411.txt -k $k " >> no_multi_kmins.txt
    ./no_multiple_kmins -src_path ./source_2000/ -query_file ./source_2000/source-document01411.txt -k $k  >> no_multi_kmins.txt
done

# test for txtalign file_length: 3000
for k in 16 32 48 64
do
    echo "./no_multiple_kmins -src_path ./source_3000/ -query_file ./source_3000/source-document03082.txt -k $k " >> no_multi_kmins.txt
    ./no_multiple_kmins -src_path ./source_3000/ -query_file ./source_3000/source-document03082.txt -k $k  >> no_multi_kmins.txt
done

# test for txtalign file_length: 4000
for k in 16 32 48 64
do
    echo "./no_multiple_kmins -src_path ./source_4000/ -query_file ./source_4000/source-document01786.txt -k $k " >> no_multi_kmins.txt
    ./no_multiple_kmins -src_path ./source_4000/ -query_file ./source_4000/source-document01786.txt -k $k  >> no_multi_kmins.txt
done

# test for txtalign file_length: 5000
for k in 16 32 48 64
do
    echo "./no_multiple_kmins -src_path ./source_5000/ -query_file ./source_5000/source-document02582.txt -k $k " >> no_multi_kmins.txt
    ./no_multiple_kmins -src_path ./source_5000/ -query_file ./source_5000/source-document02582.txt -k $k  >> no_multi_kmins.txt
done

# test for txtalign file_length: 6000
for k in 16 32 48 64
do
    echo "./no_multiple_kmins -src_path ./source_6000/ -query_file ./source_6000/source-document00712.txt -k $k " >> no_multi_kmins.txt
    ./no_multiple_kmins -src_path ./source_6000/ -query_file ./source_6000/source-document00712.txt -k $k  >> no_multi_kmins.txt
done