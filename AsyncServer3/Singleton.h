#include <mutex>
#include <memory>
template <typename T> 
class Singleton{

// 无参构造，拷贝构造，拷贝赋值  其他类无法访问
// 使用protected 子类才能访问基类的构造函数 
protected:
    Singleton() = default;
    Singleton(Singleton<T> &) = delete;
    Singleton& operator=(cosnt Singleton<T> &st) = delete;

    static std::shared_ptr<T> _instance;

// 析构函数
public:
    static std::shared_ptr<T> GetInstance(){
        static std::once_flag s_flag;
        std::call_once(s_flag, [&](){
            _instance = std::shared_ptr<T>(new T);
        });
        
        return _instance;
    }

    void PrintAddress(){
        std::cout << _instance->get() << std::endl;
    }

    ~Singleton() {
        std::cout << "this is singleton destruct" << std::endl;
    }

};

template<typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;