#ifndef _TASK_HPP_
#define _TASK_HPP_
#include<functional>
#include<future>

namespace parallel {

	template<typename T>
	class Task;

	template<typename R, typename ... Args>
	class Task<R(Args...)>
	{	
	public:
		typedef R return_type;

		//链式调用，类型推导有点骚 
		template<typename F>
		auto Then(F&& f)->Task<typename std::result_of<F(R)>::type(Args...)>
		{
			typedef typename std::result_of<F(R)>::type ReturnType;

			auto func = std::move(m_func);

			//返回的Task作为下一级函数的入参
			return Task<ReturnType(Args...)>([func, &f](Args&&... args){
				//async会等待当前的func执行完成
				std::future<R> lastf = std::async(func, std::forward<Args>(args)...);

				//lastf对象的结果作为下一级函数的入参
				return std::async(f, lastf.get()).get();
			});
		}

		Task(std::function<R(Args...)>&& f):m_func(std::move(f))
		{
		}

		Task(const std::function<R(Args...)>& f) :m_func(f)
		{
		}

		~Task()
		{
		}


		void Wait()
		{
			std::async(m_func).wait();
		}

		template<typename ... TArgs>
		R Get(TArgs &&... args)
		{
			return std::async(m_func, std::forward<TArgs>(args)...).get();
		}

		std::shared_future<R> Run()
		{
			return std::async(m_func);
		}

	private:
		std::function<R(Args...)> m_func;
	};
};




#endif // !_TASK_HPP_
