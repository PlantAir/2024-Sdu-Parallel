# Parallel Experiment （2024-Spring）
### [Teacher:weiguo Liu & zekun Yin, RA:lifeng Yan]
### [Team: kai Chen, runpei Miao, huiwu Chen]


##### 【线程生命周期优化】
0-1 任务视角的线程池
使线程可复用，减小创建和销毁线程的开销
[done] by kai chen

##### 【内存优化】
1-1 可以使用向量化打包cache block大小的数据切片
[wait] 

1-2 指针优化和cache的交互：
[ref]
可以将移入和移出的部分放在指针排序的同侧（二前/二后而非一前一后），以增加cache命中率
[wait] 

##### 【排序优化】
2-1 Block内部的排序可以针对小样本元素排序做优化
[ref]
阈值？（std：sort ？=16）个元素以下使用插入排序或其他方式
阈值？个元素以上使用归并/快排

##### 【并行优化】
3-1 初始化data时可以并行：
[ref]
使用alloc并行分配127块空间，用blockList组织整个大小为n的数组，而不是一次性单线程new整个n大小的data数组
[wait]

3-2 排序优化（2-1）比较简单归并和快排基础上
对于快排可以维护通知事件和优先队列进行优化
[wait]

##### 【编译优化】
4-1 g++ -Ofast 可以优化整整一倍
[ref]
Ofast是编译器不考虑代码text大小和C++某些编译规范，只考虑性能的编译方式
还可以使用Omax试试
[done] by kai chen