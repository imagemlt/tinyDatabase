## 红黑树实现key-value数据库

红黑树是一种平衡二叉树，具有以下性质：

* 每一个节点或者着红色，或者着黑色
* 根是黑色的
* 如果一个节点是红色的，那么它的子节点必须是黑色的
* 从一个节点到一个null指针的每一条路径必须包含相同个数的黑色节点

红黑树可以在 **O(log n)** 内的时间内进行查找、插入、删除。C++ stl中的`map`、`set`、`multimap`、`multiset`底层都是基于红黑树实现。在linux虚拟内存区域的管理以及epoll事件块管理、nginx中timer的管理都用到了红黑树。


