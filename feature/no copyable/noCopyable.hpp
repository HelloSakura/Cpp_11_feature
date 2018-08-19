#ifndef _NOCOPYABLE_HPP_
#define _NOCOPYABLE_HPP_

class NonCopyable
{
protected:
	NonCopyable() = default;
	~NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator = (const NonCopyable&) = delete;

};

#endif // !_NOCOPYABLE_HPP_
