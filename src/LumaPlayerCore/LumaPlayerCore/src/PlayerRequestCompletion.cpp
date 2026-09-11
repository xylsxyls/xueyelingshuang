#include "PlayerRequestCompletion.h"

PlayerRequestCompletion::PlayerRequestCompletion(const LumaPlayerCoreRequest& request,
    const LumaPlayerCoreCompletionCallback& callback,
    const std::shared_ptr<PlayerCompletionTask>& dispatcher) :
m_accepted(false),
m_delivery(std::make_shared<PlayerCompletionDelivery>(request, callback)),
m_dispatcher(dispatcher)
{

}

PlayerRequestCompletion::~PlayerRequestCompletion()
{
    m_delivery->m_accepted = m_accepted.load();
    m_dispatcher->publish(m_delivery);
}