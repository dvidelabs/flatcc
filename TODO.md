# TODO

- Implement buffer verification
- Follow up on leakchecks - apparently introduced some leaks recently
- Test adding null table references - also with `force_add`
- Test on Linux
- Builder: storing same vt entry twice should return error not assert
  such that parsers can check. There should also be a method to ask
  first.
- Generate `root_type` define.
