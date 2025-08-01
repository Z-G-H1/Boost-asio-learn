#include <functional>
#include <memory>
#include <mutex>
#include "Singleton.h"
#include "CSession.h"

typedef function<void(shared_ptr<CSession>, short msgid, string msg)>FunCallback;
class LogicSystem : public Singleton<LogicSystem>{
    friend class Singleton<LogicSystem>;
public:
    void PostToQue(shared_ptr<LogicNode> msg);
    ~LogicSystem();
private:
    LogicSystem();

    void Dealmsg();
    void RegisterCallbacks();
    void HelloWorldCallBack(shared_ptr<CSession> session, short msg_id, string msg_data);

    std::thread _work_thread;
    std::queue<shared_ptr<LogicNode>> _msg_que;
    std::mutex _mutex;
    std::condition_variable _consume;
    std::map<short, FunCallback> _fun_callbacks;
    bool _b_stop;
};