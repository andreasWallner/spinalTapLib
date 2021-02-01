#pragma once

namespace usb {
template <typename EF> class scope_exit {
public:
  template <typename EFP> explicit scope_exit(EFP &&f) : exit_function_(f) {}
  scope_exit(scope_exit &&rhs) noexcept : EF(rhs) {
    rhs.execute_on_destruction = false;
  }
  scope_exit(const scope_exit &) = delete;
  scope_exit &operator=(const scope_exit &) = delete;
  scope_exit &operator=(scope_exit &&) = delete;

  ~scope_exit() {
    if (execute_on_destruction_)
      exit_function_();
  }
  void release() noexcept { execute_on_destruction_ = false; }

private:
  const EF exit_function_;
  bool execute_on_destruction_{true};
};
template <typename EF> scope_exit(EF) -> scope_exit<EF>;
} // namespace usb