#pragma once

/*
	创建一个线程池
*/

#define MAX_THREAD 200

struct Task {
	int id;
	std::function<void()> fun;

	Task(int id, std::function<void()> fun) {
		this->fun = fun;
		this->id = id;
	}

	Task() {
		this->id = 0;
		this->fun = nullptr;
	};
};

class ThreadPool {

public:

	ThreadPool() {
		this->stop = false;
		this->task_queue = new std::queue<Task>();
		this->workers = new std::vector<std::thread>();
		this->runing_task = new std::set<int>();
		this->count = 0;
	}


	~ThreadPool() {

		// 这个析构条件还是有待商榷的
		{
			std::unique_lock<std::mutex> lock(this->queue_mutex);
			this->stop = true;
			this->condition.notify_all();
		}

		for (size_t i = 0; i < this->workers->size(); i++)
		{
			(*(this->workers))[i].join();
		}

		delete workers, task_queue;

	}

	void init(int num) {

		if (num <= 0 || num > MAX_THREAD) {
			return;
		}

		this->stop = false;

		for (int i = 0; i < num; i++) {
			this->workers->emplace_back(
				[this] {

					for (;;)
					{
						Task task;
						{
							std::unique_lock<std::mutex> lock(this->queue_mutex);
							// 被唤醒的条件 1 队列不空 或者 2 stop == true
							this->condition.wait(lock,
								[this] {
									return !this->task_queue->empty() || this->stop;
								}
							);

							if (this->stop == true && this->task_queue->empty()) {
								return;
							}
							//从任务队列取出一个任务
							task = std::move(this->task_queue->front());
							this->task_queue->pop();
						}

						task.fun();
						runing_task->erase(task.id);
					}

				}
			);
		}
	}

	bool get_runing(int id) {
		std::unique_lock<std::mutex> lock(this->queue_mutex);

		return 0 != this->runing_task->count(id) ? true : false;
	}

	int add(std::function<void()> fun) {

		// 使用互斥锁保护队列
		std::unique_lock<std::mutex> lock(this->queue_mutex);
		int id = this->count++;
		Task task(id, fun);
		this->task_queue->push(task);

		this->runing_task->insert(id);

		// 拉起任意一个被阻塞的线程
		this->condition.notify_one();
		
		return id;
	}


	std::vector<std::thread>* workers;
	std::queue<Task>* task_queue;

	std::mutex queue_mutex;	// 队列的互斥锁
	std::condition_variable condition;

	bool stop;
	int count;

	std::set<int>  *runing_task;
};