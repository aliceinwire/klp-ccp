#ifndef AST_IMPL_HH
#define AST_IMPL_HH

#include <type_traits>
#include "ast.hh"
#include "ast_processor_impl.hh"
#include "type_set.hh"
#include "callables_wrapper.hh"

namespace suse
{
  namespace cp
  {
    namespace ast
    {
      namespace impl
      {
	static constexpr std::size_t double_dispatch_threshold = 8;

	template <typename U>
	using parents_of = typename U::parent_types;

	template<typename callable_type, typename... args_types>
	using has_overload =
	  suse::cp::impl::has_overload<callable_type, args_types...>;

	template<typename callable_type, typename boundary_type_set>
	class stop_at_boundary_wrapper;

	template<typename callable_type, typename... boundary_types>
	class stop_at_boundary_wrapper<callable_type,
				       type_set<boundary_types...> >
	{
	public:
	  stop_at_boundary_wrapper(callable_type &&c) noexcept
	    : _c(std::forward<callable_type>(c))
	  {}

	private:
	  template<typename arg_type>
	  static constexpr bool is_boundary()
	  {
	    using boundary_type_set
	      = type_set<boundary_types...>;
	    return
	      (boundary_type_set::
	       template
	       is_member<typename std::remove_reference<arg_type>::type>());
	  }

	  template<typename arg_type>
	  static constexpr
	  typename
	  std::enable_if<has_overload<callable_type, arg_type>::value, bool>
	  ::type
	  is_void_ret()
	  {
	    using ret_type
	      = decltype(std::declval<callable_type&&>()
			 (std::declval<arg_type&&>()));
	    return std::is_same<ret_type, void>::value;
	  }

	public:
	  template<typename arg_type>
	  typename  std::enable_if<is_boundary<arg_type>(), bool>::type
	  operator()(arg_type &&arg)
	  {
	    static_assert(is_void_ret<arg_type>(),
			  "callable returns something on boundary type");
	    _c(std::forward<arg_type>(arg));
	    return false;
	  }

	  template<typename arg_type>
	  typename
	  std::enable_if<!is_boundary<arg_type>() && !is_void_ret<arg_type>(),
			 bool>::type
	  operator()(arg_type &&arg)
	  {
	    static_assert(!is_boundary<arg_type>(), "implementation error");
	    return _c(std::forward<arg_type>(arg));
	  }

	  template<typename arg_type>
	  typename
	  std::enable_if<!is_boundary<arg_type>() && is_void_ret<arg_type>(),
			 bool>::type
	  operator()(arg_type&&)
	  {
	    static_assert(!is_boundary<arg_type>() && is_void_ret<arg_type>(),
			  "implementation error");
	    static_assert(1, "void returned on non-boundary ast entity");
	  }

	private:
	  callable_type &&_c;
	};
      }


      template <typename callable_type>
      void _ast_entity::for_each_dfs_po(callable_type &&c)
      {
	  for (std::size_t i_child = 0;; ++i_child) {
	    _ast_entity * const ae = _get_child(i_child);
	    if (!ae)
	      break;
	    ae->for_each_dfs_po(std::forward<callable_type>(c));
	  }

	  c(*this);
      }

      template<typename callable_type_pre,
	       typename callable_type_post>
      void _ast_entity::for_each_dfs_pre_and_po(callable_type_pre &&c_pre,
						callable_type_post &&c_post)
      {
	const bool do_po = c_pre(*this);

	for (std::size_t i_child = 0;; ++i_child) {
	  _ast_entity * const ae = _get_child(i_child);
	  if (!ae)
	    break;
	  ae->for_each_dfs_pre_and_po(c_pre, c_post);
	}

	if (do_po)
	  c_post(*this);
      }


      template<typename derived>
      template<typename boundary_type_set, typename callable_type>
      void
      ast_entity<derived>::for_each_ancestor(callable_type &&c)
      {
	impl::stop_at_boundary_wrapper<callable_type, boundary_type_set> _c
	  (std::forward<callable_type>(c));

	typedef
	  typename
	  derived::parent_types::template closure<impl::parents_of,
						  boundary_type_set>
	  ancestor_types;

	auto &&__c =
	  wrap_callables<default_action_unreachable<bool, ancestor_types>
			 ::template type>
	  (_c);
	_ast_entity *p = _parent;
	if (ancestor_types::size() < impl::double_dispatch_threshold) {
	  while (p) {
	    if (!ancestor_types::cast_and_call(__c, *p))
	      return;

	    p = p->get_parent();
	  }
	} else {
	  auto &&processor = make_processor<bool>(__c);
	  while (p) {
	    if (!p->_process(processor))
	      return;
	    p = p->get_parent();
	  }
	}
      }


      template <typename handled_types, typename callables_wrapper_type>
      void ast::for_each_dfs_po(callables_wrapper_type &&c)
      {
	static_assert((handled_types::size() ==
		       (std::remove_reference<callables_wrapper_type>::type::
			size())),
		      "number of overloads != number of handled types");

	if (!_tu)
	  return;

	if (handled_types::size() < impl::double_dispatch_threshold) {
	  auto &&_c = [&c](_ast_entity &ae) {
	    handled_types::cast_and_call(c, ae);
	  };
	  _tu->for_each_dfs_po(_c);
	} else {
	  auto &&processor = make_processor<void>(c);
	  auto &&_c = [&processor](_ast_entity &ae) {
	    ae._process(processor);
	  };
	  _tu->for_each_dfs_po(_c);
	}
      }

      template <typename handled_types_pre,
		typename handled_types_post,
		typename callables_wrapper_type_pre,
		typename callables_wrapper_type_post>
      void ast::for_each_dfs_pre_and_po(callables_wrapper_type_pre &&c_pre,
					callables_wrapper_type_post &&c_post)
      {
	static_assert((handled_types_pre::size() ==
		       (std::remove_reference<callables_wrapper_type_pre>
			::type::size())),
		      "pre: number of overloads != number of handled types");
	static_assert((handled_types_post::size() ==
		       (std::remove_reference<callables_wrapper_type_post>
			::type::size())),
		      "post: number of overloads != number of handled types");
	if (!_tu)
	  return;

	if (handled_types_pre::size() < impl::double_dispatch_threshold &&
	    handled_types_post::size() < impl::double_dispatch_threshold) {
	  auto &&_c_pre = [&c_pre](_ast_entity &ae) {
	    return handled_types_pre::cast_and_call(c_pre, ae);
	  };
	  auto &&_c_post = [&c_post](_ast_entity &ae) {
	    handled_types_post::cast_and_call(c_post, ae);
	  };
	  _tu->for_each_dfs_pre_and_po(_c_pre, _c_post);

	} else if (handled_types_pre::size() <
		   impl::double_dispatch_threshold) {
	  auto &&_c_pre = [&c_pre](_ast_entity &ae) {
	    return handled_types_pre::cast_and_call(c_pre, ae);
	  };
	  auto &&processor_post = make_processor<void>(c_post);
	  auto &&_c_post = [&processor_post](_ast_entity &ae) {
	    ae._process(processor_post);
	  };
	  _tu->for_each_dfs_pre_and_po(_c_pre, _c_post);

	} else if (handled_types_post::size() <
		   impl::double_dispatch_threshold) {
	  auto &&processor_pre = make_processor<bool>(c_pre);
	  auto &&_c_pre = [&processor_pre](_ast_entity &ae) {
	    return ae._process(processor_pre);
	  };
	  auto &&_c_post = [&c_post](_ast_entity &ae) {
	    handled_types_post::cast_and_call(c_post, ae);
	  };
	  _tu->for_each_dfs_pre_and_po(_c_pre, _c_post);

	} else {
	  auto &&processor_pre = make_processor<bool>(c_pre);
	  auto &&_c_pre = [&processor_pre](_ast_entity &ae) {
	    return ae._process(processor_pre);
	  };
	  auto &&processor_post = make_processor<void>(c_post);
	  auto &&_c_post = [&processor_post](_ast_entity &ae) {
	    ae._process(processor_post);
	  };
	  _tu->for_each_dfs_pre_and_po(_c_pre, _c_post);

	}
      }
    }
  }
}

#endif
