# ming_OS
### 在https://github.com/cfenollosa/os-tutorial 的基础上，进一步实现一个简洁的操作系统
### 配合《深入理解计算机系统》食用
---------------
### 开篇的啰嗦
os-tutorial确实为我学习操作系统打下了非常好的基础，它让我对系统的底层有了深刻的认知。但是非常可惜的是在完成了系统的中断之后，os-tutorial就太监了。如果你对我的这个项目感兴趣，而对操作系统的底层还没有多少认知的话，可以看下我的笔记：[os-tutorial笔记](https://github.com/isyiming/live-up/OS/Readme.md)

我们还没有看到操作系统的虚拟内存，文件和进程是如何实现的。这样的系统还没有灵魂。如果想要对操作系统的概念有一个全面系统的认知，没有比自己动手实现一下更合适的方法了。虽然CSAPP这本神书在每一节后都会有对应的习题，但是那些习题真的有点碎片化了，而且做起来实在是太习题了，我总是感觉没有融入工程。这个项目就是要在os-tutorial的基础上，继续完善，最终实现一个真正意义上操作系统。


本项目的最初代码来源于os-tutorial的第23节，开发环境：macOS catalina, 需要你安装i386-gcc等开发环境，安装方法在我的笔记里：[系统的启动和引导内核](https://github.com/isyiming/live-up/blob/master/OS/OSpart1.md)

为了方便，本项目和常见系统会有些不同的地方。
1.一般系统的启动是：bios加载磁盘的第一个扇区的引导程序，引导程序再加载内核代码。这时候内核代码一般在0x100000（1MB）内存地址位置处开始。但是本项目直接把内核和引导程序放在一起了。

#### [part1：对物理内存分页](https://github.com/isyiming/ming_OS/blob/master/part1.md)
#### [part2：分页管理和缺页异常](https://github.com/isyiming/ming_OS/blob/master/part2.md)



要是想做成一个可以真正在机器上运行的系统，就需要制作一个预先加载了grub的映像文件。
方法在这里：http://blog.sina.com.cn/s/blog_70dd169101013gcw.html
有时间尝试下。
