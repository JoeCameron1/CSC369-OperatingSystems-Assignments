README.txt
Assignment 3
Joseph Cameron (camer249)


1. Task 2 Tables

The first three programs are simpleloop, matmul and blocked. 

The fourth program (the program of my choice) is a python program from CSC384 A1, which involves CSP (Constraint Satisfaction Problems) Problems.
I have chosen this program because it could show some interesting behaviour.

—————————————————————————————————————————————————————————
—————————————————————————————————————————————————————————
Trace file = tr-simpleloop.ref

Memory Size =  50

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     72.6905        8089        3039              2989                   277                  2712
  FIFO     73.2297        8149        2979              2929                   206                  2723
   LRU     74.9730        8343        2785              2735                    88                  2647
 CLOCK     74.8382        8328        2800              2750                    96                  2654
   OPT     75.7576        8425        2696              2657                    23                  2634

—————————————————————————————————————————————————————————

Memory Size =  100

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     74.9641        8342        2786              2686                    73                  2613
  FIFO     75.1887        8367        2761              2661                    44                  2617
   LRU     75.8357        8439        2689              2589                     2                  2587
 CLOCK     75.8088        8436        2692              2592                     3                  2589
   OPT     76.2881        8484        2637              2544                     0                  2544

—————————————————————————————————————————————————————————

Memory Size =  150

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     75.4853        8400        2728              2578                    20                  2558
  FIFO     75.5572        8408        2720              2570                    16                  2554
   LRU     75.8627        8442        2686              2536                     0                  2536
 CLOCK     75.8537        8441        2687              2537                     0                  2537
   OPT     76.2850        8489        2639              2489                     0                  2489

—————————————————————————————————————————————————————————

Memory Size =  200

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     75.6470        8418        2710              2510                    16                  2494
  FIFO     75.6290        8416        2712              2512                    12                  2500
   LRU     75.8627        8442        2686              2486                     0                  2486
 CLOCK     75.8627        8442        2686              2486                     0                  2486
   OPT     76.2850        8489        2639              2439                     0                  2439

—————————————————————————————————————————————————————————

—————————————————————————————————————————————————————————
—————————————————————————————————————————————————————————
Trace file = tr-matmul.ref

Memory Size =  50

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     65.5255     1892818      995854            995804                956318                 39486
  FIFO     60.9766     1761415     1127257           1127207               1083214                 43993
   LRU     63.9551     1847452     1041220           1041170               1040063                  1107
 CLOCK     63.9549     1847448     1041224           1041174               1040062                  1112
   OPT     80.2083     2380821      587479            587415                586329                  1086

—————————————————————————————————————————————————————————

Memory Size =  100

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     88.8008     2565163      323509            323409                315974                  7435
  FIFO     62.4898     1805124     1083548           1083448               1061223                 22225
   LRU     65.1586     1882218     1006454           1006354               1005274                  1080
 CLOCK     65.3199     1886878     1001794           1001694               1000612                  1082
   OPT     96.8735     2875485       92803             92699                 91619                  1080     2968288

—————————————————————————————————————————————————————————

Memory Size =  150

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     96.6826     2792842       95830             95680                 93386                  2294
  FIFO     98.8089     2854264       34408             34258                 32943                  1315
   LRU     98.8616     2855787       32885             32735                 31656                  1079
 CLOCK     98.6047     2848367       40305             40155                 39075                  1080
   OPT     99.1033     2941672       26616             26466                 25383                  1083

—————————————————————————————————————————————————————————

Memory Size =  200

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     98.0462     2832232       56440             56240                 54606                  1634
  FIFO     98.8269     2854785       33887             33687                 32433                  1254
   LRU     98.8620     2855799       32873             32673                 31594                  1079
 CLOCK     98.8615     2855785       32887             32687                 31608                  1079
   OPT     99.3509     2949022       19266             19060                 17983                  1077

—————————————————————————————————————————————————————————

—————————————————————————————————————————————————————————
—————————————————————————————————————————————————————————
Trace file = tr-blocked.ref

Memory Size =  50

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     99.6567     2410552        8304              8254                  5712                  2542
  FIFO     99.7345     2412433        6423              6373                  4100                  2273
   LRU     99.7878     2413724        5132              5082                  2746                  2336
 CLOCK     99.7629     2413120        5736              5686                  3249                  2437
   OPT     99.8534     2521370        3702              3652                  2568                  1084

—————————————————————————————————————————————————————————

Memory Size =  100

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     99.7823     2413590        5266              5166                  3402                  1764
  FIFO     99.8220     2414550        4306              4206                  2727                  1479
   LRU     99.8436     2415072        3784              3684                  2603                  1081
 CLOCK     99.8303     2414752        4104              4004                  2609                  1395
   OPT     99.8812     2522072        3000              2900                  1829                  1071

—————————————————————————————————————————————————————————

Memory Size =  150

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     99.8201     2414505        4351              4201                  2739                  1462
  FIFO     99.8260     2414648        4208              4058                  2636                  1422
   LRU     99.8442     2415088        3768              3618                  2558                  1060
 CLOCK     99.8437     2415076        3780              3630                  2570                  1060
   OPT     99.9000     2522547        2525              2375                  1300                  1075

—————————————————————————————————————————————————————————

Memory Size =  200

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     99.8427     2415050        3806              3606                  2281                  1325
  FIFO     99.8693     2415694        3162              2962                  1865                  1097
   LRU     99.8472     2415161        3695              3495                  2435                  1060
 CLOCK     99.8677     2415655        3201              3001                  1941                  1060
   OPT     99.9099     2522797        2275              2075                  1009                  1066

—————————————————————————————————————————————————————————

—————————————————————————————————————————————————————————
—————————————————————————————————————————————————————————
Trace file = myprogram.ref

Memory Size =  50

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     99.7021      449166        1342              1272                   860                   412
  FIFO     99.7461      449443        1144              1092                   710                   382
   LRU     99.8304      449747         764               694                   406                   288
 CLOCK     99.8275      449777         777               710                   420                   290
   OPT     99.8943      450200         476               427                   173                   254

—————————————————————————————————————————————————————————

Memory Size =  100

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     99.9001      450024         450               350                   115                   235
  FIFO     99.9094      450080         408               306                    71                   235
   LRU     99.9290      450172         320               218                    34                   184
 CLOCK     99.9303      450175         314               215                    33                   182
   OPT     99.9467      450249         240               141                     2                   139

—————————————————————————————————————————————————————————

Memory Size =  150

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     99.9292      450164         319               168                    22                   146
  FIFO     99.9458      450241         244                95                     0                    95
   LRU     99.9501      450263         225                75                     0                    75
 CLOCK     99.9498      450262         226                76                     0                    76
   OPT     99.9501      450268         225                75                     0                    75

—————————————————————————————————————————————————————————

Memory Size =  200

Algorithm  Hit Rate    Hit Count   Miss Count  Overall Eviction Count  Clean Eviction Count  Dirty Eviction Count

  RAND     99.9432      450233         255                55                     3                    52
  FIFO     99.9492      450259         229                29                     0                    29
   LRU     99.9501      450263         225                25                     0                    25
 CLOCK     99.9496      450261         226                26                     0                    26
   OPT     99.9501      450268         225                25                     0                    25

—————————————————————————————————————————————————————————



2. Comparison of hit rates.

In order to compare the various algorithms, we will compare the hit rate for various memory sizes. Generally, we seem to have obtained a trend of: FIFO hit rate < LRU hit rate ~= CLOCK hit rate < OPT hit rate. OPT seems to always have the highest hit rate. This makes sense since OPT is meant to be optimal and OPT always selects the page that won't be used for the longest period of time. Thus when a page is being evicted, OPT ensures that it’s the best page to pick. This minimises the miss count in the entire process. RAND appears to do better than FIFO, LRU, CLOCK for the tr-matmul trace file because the data used in matmul is randomly/dynamically generated. This means the blocks of data are less likely to be contiguous. The LRU and CLOCK can't optimise too much with locality reduced, therefore RAND might get the chance to perform better. RAND appears to do worse than FIFO, LRU, CLOCK in the other trace files, where locality of data is ensured by the program. By similar reasoning, FIFO, LRU, CLOCK can make use of locality to reduce miss count, beating RAND in this case. In tr-simpleloop, RAND does obviously the worse since the trace file exhibits strong locality and RAND can't utilise this property. LRU and CLOCK get very similar hit rates in all cases, mainly because they are very similar algorithms. So, we can see CLOCK (evicting the page that is old enough) mimicking LRU (evicting the oldest page). LRU and CLOCK both perform better than FIFO since they deal with the last reference time for a frame rather than the first reference time.


3. LRU

In general for each test trace file, the hit rate (or hit count) increases as memory size increases. LRU makes use of the fact that program and data references within a process tend to cluster in physical memory. Once eviction is needed, LRU selects the page that hasn't been used for the longest time. With increased memory size, we can store more recently used pages, which enables us to store a more complete profile of recently referenced pages, improving the precision of our knowledge with regards to the least recently used page. Hence, LRU is good at exploiting the locality of a program by analysing the past, leading to increased hit rate. When memsize increased from 100 to 150 for the matmul case, there is a sudden jump in hit rate. With some calculation, we can see for storing each matrix (to be multiplied), we need roughly 100 page frames. And for naive multiplication to work, we need to have 200 page frames for storing the two matrices. However, for the case where memsize = 50 or 100, it is far from enough to load the matrix all at once, hence switching back and forth between the 200 frames is needed. With memsize = 150, we can store at least one and a half matrices at the same time, reducing a lot of page replacement in computation. This essentially means LRU is close to its computational best at a memsize of 150 as we can see from the hit rates at a memsize of 200 being very similar to the hit rate at a memsize of 150.
