#ifndef SE_INCL_TIMER_INL
#define SE_INCL_TIMER_INL
#ifdef PRAGMA_ONCE
  #pragma once
#endif

/* Constructor from seconds. */
inline CTimerValue::CTimerValue(SECOND fSeconds)
{
  tv_llValue = SQUAD(fSeconds * (SECOND)_llTimerValueSecondLen);
}
/* Clear timer value (set it to zero). */
inline void CTimerValue::Clear(void)
{
  tv_llValue = 0;
}
/* Addition. */
inline CTimerValue &CTimerValue::operator+=(const CTimerValue &tvOther) {
  tv_llValue+=tvOther.tv_llValue;
  return *this;
};
inline CTimerValue CTimerValue::operator+(const CTimerValue &tvOther) const {
  return CTimerValue(*this)+=tvOther;
};
/* Substraction. */
inline CTimerValue &CTimerValue::operator-=(const CTimerValue &tvOther) {
  tv_llValue-=tvOther.tv_llValue;
  return *this;
};
inline CTimerValue CTimerValue::operator-(const CTimerValue &tvOther) const {
  return CTimerValue(*this)-=tvOther;
};
/* Comparisons. */
inline BOOL CTimerValue::operator<(const CTimerValue &tvOther) const {
  return tv_llValue<tvOther.tv_llValue;
}
inline BOOL CTimerValue::operator>(const CTimerValue &tvOther) const {
  return tv_llValue>tvOther.tv_llValue;
}
inline BOOL CTimerValue::operator<=(const CTimerValue &tvOther) const {
  return tv_llValue<=tvOther.tv_llValue;
}
inline BOOL CTimerValue::operator>=(const CTimerValue &tvOther) const {
  return tv_llValue>=tvOther.tv_llValue;
}
/* Get the timer value in seconds. - use for time spans only! */
inline SECOND CTimerValue::GetSeconds(void) {
  return SECOND(tv_llValue) / SECOND(_llTimerValueSecondLen);
};
/* Get the timer value in milliseconds as integral value. */
inline SQUAD CTimerValue::GetMilliseconds(void) {
  return tv_llValue / (_llTimerValueSecondLen / 1000);
};


#endif  /* include-once check. */

