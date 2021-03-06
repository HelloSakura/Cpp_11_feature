１、结构划分：
同步服务层：处理来自上层的任务请求，上层的请求可能是并发的，任务放到一个同步队列中，等待处理
排队层：来自上层的任务请求都会加到排队层中等待处理
异步服务层：多个线程同时处理排队层中的任务


2、 实现：其实并不是很复杂，首先需要一个同步队列，这个队列负责存储任务，同时对每个就可能出现互斥访问的地方加锁实现同步操作；然后实现线程池，线程池一般通过shared_ptr指针保存各个线程，同时线程池需要实现线程的方法体，这个方法体的主要任务就是从同步队列中领任务（执行完了，继续领，直到stop）；目前这个半同步半异步线程池已经实现的差不多了，剩下的就是来自同步层的，同步层通过ThreadPool来添加任务，将任务交给ThreadPool中的线程来处理，任务通过同步队列来管理，我更愿意将这一过程说成委托，任务委托给服务线程。

3、一些小细节：
①同步队列每一个异步操作的函数都要加锁，比较重要的是在Add，即添加Task的时候使用加锁操作；
②unique_lock和lock_guard的不同，前者随时可以释放锁，而后者只在析构时释放锁；
③更关心的是，多个线程是如何使用这个同步队列的，同步服务是来自外部的请求，通过threadPool委托任务，任务由同步队列来管理

	
	
	
4、关于实现上的一些问题：
①左值与右值引用：左值引用相当于一个变量的别名，通过别名可以直接对其对应的变量进行值操作；而右值引用则是关联到右值的，一点个人的理解，由于右值的生命期很短，比如临时变量，基本在一条语句之内就完成了创建销毁操作。按照定义而言的话，右值不能取地址，没有名字的值；比如一些表达式返回的临时对象，函数返回值（返回的不是引用），之后为了优化程序性能（其中很突出一点是，对象拷贝复制时的性能优化，具体可以参加这篇博客：http://www.cnblogs.com/catch/p/3507883.html  很赞）
     而右值引用出现的一个突出的作用是实现移动语义，当发生拷贝的时候，如果知道传来的是一个临时值，以往的做法往往是通过临时值创建临时对象，将这个对象赋值给我们需要的对象，然后依次进行销毁工作；但是如果我们直接将临时值的资源进行一下移动，将它交给我们需要赋值的对象，性能不就得到了提高，这就是移动语义的来处。

②移动语义：实际文件还在原来位置，而只修改记录（理解为：转让资源的所有权）
    移动语义的目的在上面已经做过介绍，避免大量无用的构造析构工作。如何实现移动语义，一般而言提供需要编写移动构造函数，这点之后再讲。在此补充一下关于std::move语义，实际上就是强制将一个左值转换成一个右值，至于深度了解可以参照上文的链接。

③完美转发std::forward：在函数模版中，完全依照函数模板的参数类型，保持参数的左值、右值特征，将参数转递给函数模版中调用的另一个函数。
     同时介绍一下在参数类型推导上的引用折叠原则：所有的右值引用叠加到右值引用上仍然还是一个右值引用；所有的其它引用类型之间的叠加都将变成左值引用。即T&& && => T&& , T&& & = T&

④一个性能高的类需要哪几种构造函数：主要是在复制构造函数的时候，涉及到对象的深浅拷贝的时候（尤其对象使用的堆内存比较大的情况下），一般而言我们只会提供对象的拷贝构造函数和赋值构造函数；但是拷贝临时对象时，难以避免多次的构造析构；结合上面的，提供右值引用版本的拷贝赋值构造函数，在内部完成移动过语义，优化性能。


    
    
