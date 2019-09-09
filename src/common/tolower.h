#pragma once

static constexpr char ToLower(char c)
{
    return ((c >= 'A') & (c <= 'Z')) ? (c + ('a' - 'A')) : (c);
}
