#include "CookReviewRevision.h"
#include "RapidJson/RapidJsonAPI.h"

CookReviewRevision::CookReviewRevision() :
    m_submissionNumber(0),
    m_reviewNumber(0),
    m_submittedAtSeconds(0),
    m_reviewedAtSeconds(0),
    m_decisionSequence(0),
    m_requiresReview(false)
{

}

RapidJsonValue CookReviewRevision::toJson(bool withDelta) const
{
    RapidJsonValue value;
    value.setObject();
    value.addString("id", m_id);
    value.addString("recipeId", m_recipeId);
    value.addString("ownerUserId", m_ownerUserId);
    value.addString("parentId", m_parentId);
    value.addString("previousApprovedId", m_previousApprovedId);
    if (withDelta)
    {
        value.addString("deltaJson", m_deltaJson);
    }
    value.addString("state", m_state);
    value.addString("reviewerUserId", m_reviewerUserId);
    value.addString("reason", m_reason);
    value.addInt("submissionNumber", m_submissionNumber);
    value.addInt("reviewNumber", m_reviewNumber);
    value.addInt("submittedAtSeconds", m_submittedAtSeconds);
    value.addInt("reviewedAtSeconds", m_reviewedAtSeconds);
    value.addInt("decisionSequence", m_decisionSequence);
    value.addBool("requiresReview", m_requiresReview);
    return value;
}

bool CookReviewRevision::loadJson(const std::string &json)
{
    RapidJsonDocument value;
    if (!value.parse(json) || !value.isObject())
    {
        return false;
    }
    m_id = value.getStringOrDefault("id", "");
    m_recipeId = value.getStringOrDefault("recipeId", "");
    m_ownerUserId = value.getStringOrDefault("ownerUserId", "");
    m_parentId = value.getStringOrDefault("parentId", "");
    m_previousApprovedId = value.getStringOrDefault("previousApprovedId", "");
    m_deltaJson = value.getStringOrDefault("deltaJson", "");
    m_state = value.getStringOrDefault("state", "");
    m_reviewerUserId = value.getStringOrDefault("reviewerUserId", "");
    m_reason = value.getStringOrDefault("reason", "");
    m_submissionNumber = value.getIntOrDefault("submissionNumber", 0);
    m_reviewNumber = value.getIntOrDefault("reviewNumber", 0);
    m_submittedAtSeconds = value.getIntOrDefault("submittedAtSeconds", 0);
    m_reviewedAtSeconds = value.getIntOrDefault("reviewedAtSeconds", 0);
    m_decisionSequence = value.getIntOrDefault("decisionSequence", 0);
    m_requiresReview = value.getBoolOrDefault("requiresReview", false);
    return !m_id.empty() && !m_recipeId.empty() && !m_ownerUserId.empty() && !m_deltaJson.empty() &&
           m_submissionNumber > 0 && m_reviewNumber >= 0 && m_submittedAtSeconds > 0 && m_reviewedAtSeconds >= 0 &&
           m_decisionSequence >= 0 &&
           (m_state == "pending" || m_state == "approved" || m_state == "rejected" || m_state == "superseded");
}