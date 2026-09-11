#pragma once

using namespace geode::prelude;

extern std::string m_primary_url;
extern std::string m_fallback_url;
extern std::string m_value;

void applyServerEndpoint();
void openURL(const char* psz);