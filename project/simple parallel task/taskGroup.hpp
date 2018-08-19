#ifndef _TASKGROUP_CPP_
#define _TASKGROUP_CPP_


#include<vector>
#include<map>
#include<string>
#include<future>

#include"variant.hpp"
#include"any.hpp"
#include"noCopyable.hpp"
#include"task.hpp"

namespace parallel {
	class TaskGroup: NonCopyable
	{
		
		typedef Variant<int, std::string, double, short, unsigned int> RetVariant;
	public:
		TaskGroup()
		{}

		~TaskGroup()
		{}


		template<typename R, typename = typename std::enable_if<!std::is_same<R, void>::value>::type>
		void Run(Task<R()>&& task)
		{
			m_group.emplace(R(), task.Run());
		}

		void Run(Task<void()>&& task)
		{
			m_voidGroup.push_back(task.Run());
		}

		template<typename F>
		void Run(F&& f)
		{	
			Run(Task<std::result_of<F()>::type()>(std::forward<F>(f)));
		}

		template<typename F, typename ... Funs>
		void Run(F&& first, Funs&&... rest)
		{
			Run(std::forward<F>(first));
			Run(std::forward<Funs>(rest)...);
		}

		void Wait()
		{
			for (auto it = m_group.begin(); it != m_group.end(); ++it) {
				auto vrt = it->first;
				vrt.visit(
					[&](int a) {
						FutureGet<int>(it->second);
					},

					[&](double b) {
						FutureGet<double>(it->second);
					},

					[&](std::string v) {
						FutureGet<std::string>(it->second);
					},

					[&](short s) {
						FutureGet<short>(it->second);
					},

					[&](unsigned int s) {
						FutureGet<unsigned int>(it->second);
					}
				);
			}

			for (auto it = m_voidGroup.begin(); it != m_voidGroup.end(); ++it) {
				it->get();
			}
		}

	private:
		template<typename T>
		void FutureGet(Any& f)
		{
			f.AnyCast<std::shared_future<T>>().get();
		}

	private:
		std::multimap<RetVariant, Any> m_group;
		std::vector<std::shared_future<void>> m_voidGroup;
	};

}


#endif // !_TASKGROUP_CPP_
