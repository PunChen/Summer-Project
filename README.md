# Summer-Project
最开始几天的工作没有往github上面写，本来想的是git上面只是放代码，后来商量了一下还是决定也写一下思路和过程。
### Update 2020.06.15
前几天的工作都写在这里了。
### 数据下载
这几天下载数据花了不少的时间,因为数据太大了,21号染色体的基因信息有50G.现在放到了服务器上,本地切下来一小部分用于初步的项目调试.最后在应用大数据进行关联分析.
其中dataxxx.bgen是基因数据,ukb_imp_chr_v3.sample是表形数据.后续的工作就是解析两种数据并进行关联分析.
具体的,sample是csv格式,首行存储列名,第二行存储数据类型.不同列分别存储了ID,表型,临床变量等信息.

完整的21号染色体数据中共包含487409个个体的1261158个SNPs,这里的所有.bgen文件来自所有样本的21号染色体的变异信息,每个文件包含10个SNPs.

由于染色体上的基因数据和变异太多了，存储是个大问题，所以采用压缩算法是十分有必要的，而且最好的无损的压缩算法。

现在下载好的21号染色体基因数据是经过zstd压缩的，要想解压缩，就要先了解这一种数据压缩算法。

zstd是Facebook在2016年开源的新无损压缩算法，优点是压缩率和压缩/解压缩性能都很突出。
在我们测试的文本日志压缩场景中，压缩率比gzip提高一倍，压缩性能与lz4、snappy相当甚至更好，是gzip的10倍以上。
zstd还有一个特别的功能，支持以训练方式生成字典文件，相比传统压缩方式能大大的提高小数据包的压缩率。
在过去的两年里，Linux内核、HTTP协议、以及一系列的大数据工具（包括Hadoop 3.0.0，HBase 2.0.0，Spark 2.3.0，Kafka 2.1.0）等都已经加入了对zstd的支持。
可以预见，zstd将是未来几年里会被广泛关注和应用的压缩算法。

### 环境配置
首先优化的话必不可少的是并行和高性能的数学库，并行暂时准备使用linux系统自带的pthreads和openmp，高性能数学库使用intel的mkl。

最近一直卡在intel编译器的配置上，直接一键安装了intel的parallel_studio_xe_2019，里面带有icc icpc mkl等，这里简单说一下配置过程：
首先去intel官网下载parallel_studio_xe_2019linux压缩包（会用到学生邮箱获取序列号），本地解压之后直接运行install.sh，按照提示一步步安装。
注意可能需要安装一些其他环境。
服务器上也是按照这个过程安装。
最终测试了一个vector的程序来比较g++和icpc在性能上的差异：
  ![icpcTest](https://github.com/PunChen/Summer-Project/blob/master/imgs/2020-06-12%2010-11-26%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)
一个90s 一个2s，可以看到性能的提升还是十分明显的。

### Update 2020.06.24
更新了线性回归的相关知识，为基因数据的筛选打下来基础

### Update 2020.06.25
### BgenParser

这里用的是BGEN v1.2，它相比之前的版本做了一些改进，具体如下：

支持可变倍性和显式缺失数据。
支持多等位基因变体（例如复杂的结构变体）。
通过支持以可配置精度存储的基因型概率来控制文件大小。
支持存储样本标识符。

解析这种数据就必须找官方的文档参考了，因为初始文件是二进制，不像sample一样可以直接看到数据。

参考官方文档对其的解释：

![image-20200623093219981](C:\Users\yanli\AppData\Roaming\Typora\typora-user-images\image-20200623093219981.png)

一个BGEN文件由一个标题块和一个可选的样本标识符块组成，该标题块提供有关该文件的常规信息。这些之后是一系列连续存储在文件中的变体数据块，每个变体数据块均包含单个遗传变体的数据。

具体的：

![image-20200623093339285](C:\Users\yanli\AppData\Roaming\Typora\typora-user-images\image-20200623093339285.png)

即前4个字节存了一个偏移量，表示真正的数据（跳过标题块）从哪里开始（相对第5个字节的偏移量）。这样我们就能确定下标题块的大小以及数据块的位置。

然后根据标题块的详细描述对标题块先进性解析：

![image-20200623095758070](C:\Users\yanli\AppData\Roaming\Typora\typora-user-images\image-20200623095758070.png)

![image-20200623100622723](C:\Users\yanli\AppData\Roaming\Typora\typora-user-images\image-20200623100622723.png)

首先读出偏移量offset，然后读整个标题块，如果满足条件，就具体的读具体的样本表示块，最后把指针跳到具体的变异数据块。下面是这部分的输出：

![image-20200623100748543](C:\Users\yanli\AppData\Roaming\Typora\typora-user-images\image-20200623100748543.png)

下面是具体的变异块的解析，还是按照官方的文档摸清楚他的文件结构

![image-20200623102119851](C:\Users\yanli\AppData\Roaming\Typora\typora-user-images\image-20200623102119851.png)



最终，基本理清楚每部分数据是干嘛的之后，把sample数据和bgen数据合并。

大体的思路是，首先50w个人，没人一行，然后每一行存这个人的相关信息，这里为了减少对内存的要求，并不是一次性把整个bgen文件读进来操作，而是切成一块一块的。

整个21号染色体的变异信息有37G，总共1261158条变异信息，调试部分用的数据是4个包含10条变异的bgen文件：

![image-20200625111605772](C:\Users\yanli\AppData\Roaming\Typora\typora-user-images\image-20200625111605772.png)

初步定下运行单个文件包含100条变异，后期在跑在更大的数据上：

![image-20200625111917792](C:\Users\yanli\AppData\Roaming\Typora\typora-user-images\image-20200625111917792.png)

经历数据解析之后的sample格式大体如下：

<img src="file:///D:\QQ\QQData\1362520198\Image\C2C\57991DA68DC1169DBB6617F56A935994.png" alt="img" style="zoom:50%;" />

解析好的bgen格式（某一个变异）：

<img src="file:///D:\QQ\QQData\1362520198\Image\C2C\B4F467954094447EDFFB7C44B04B2AA6.png" alt="img" style="zoom:50%;" />

合并之后用于计算的数据格式：

<img src="file:///D:\QQ\QQData\1362520198\Image\C2C\63C7ED3C996356EE1D6346423EC40BC6.png" alt="img" style="zoom:50%;" />

其中48w就是数据中的48w（接近50w）个人，pcasNum就是输入的协变量个数，+1是要分析的表型。

至此数据解析部分就全部完成了，不过没有加并行来优化，和之前的理由一样，整个程序的热点不一定在这里，而且很大的可能性不在这里，所以优化的重心放在后面计算部分。

核心代码：

![image-20200625113019026](C:\Users\yanli\AppData\Roaming\Typora\typora-user-images\image-20200625113019026.png)

![image-20200625113044875](C:\Users\yanli\AppData\Roaming\Typora\typora-user-images\image-20200625113044875.png)

运行结果：

![image-20200625113140956](C:\Users\yanli\AppData\Roaming\Typora\typora-user-images\image-20200625113140956.png)

解析部分的热点实在解压缩和解析bgen文件上，因为他采用的高效的压缩算法，所以解压和解析起来也有点麻烦。具体的，跑在一个调试用的小文件（10个变异，17M）下，大约3秒。
