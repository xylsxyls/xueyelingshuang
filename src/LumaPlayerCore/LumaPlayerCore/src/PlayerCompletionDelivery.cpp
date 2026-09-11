#include "PlayerCompletionDelivery.h"

PlayerCompletionDelivery::PlayerCompletionDelivery(const LumaPlayerCoreRequest& request,
    const LumaPlayerCoreCompletionCallback& callback) :
m_callback(callback),
m_accepted(false),
m_ready(false)
{
    m_completion.m_request = request;
}