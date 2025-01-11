#pragma once

// Return the smallest index i in [first, last) at which pred(i) is true.
// Returns last if not found.
template <typename Idx, typename Pred>
Idx wicked_upper_bound(Idx first, Idx last, const Pred& pred) {
  Idx i = first;
  Idx n = last - first;
  while (n > 0) {
    Idx h = n / 2;
    if (pred(i + h)) {
      n = h;
    } else {
      i += h + 1;
      n -= h + 1;
    }
  }
  return i;
}
