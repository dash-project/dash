
#ifndef DASH_MATRIX_H_INCLUDED
#define DASH_MATRIX_H_INCLUDED

#include <type_traits>
#include <stdexcept>

#include "Team.h"
//#include "Pattern.h"
#include "Pattern.h"
#include "GlobIter.h"
#include "GlobRef.h"
#include "HView.h"

#include "dart.h"

namespace dash {

	template <typename T, size_t DIM> class Matrix;

	template <typename T, size_t DIM, size_t CUR> class Matrix_Ref;
	template <typename T, size_t DIM, size_t CUR> class Local_Ref;

	template <typename T, size_t DIM> class Matrix_RefProxy {

	private:
		int m_dim = 0;
		Matrix<T, DIM> *m_mat;
		std::array<long long, DIM> m_coord;
		ViewSpec<DIM> m_viewspec;

	public:
		template<typename T_, size_t DIM1, size_t DIM2> friend class Matrix_Ref;
		template<typename T_, size_t DIM1, size_t DIM2> friend class Local_Ref;
		template<typename T_, size_t DIM1> friend class Matrix;

		Matrix_RefProxy<T, DIM>() = default;
	};

	template <typename T, size_t DIM, size_t CUR = DIM> class Local_Ref {

	private:


	public:
		template<typename T_, size_t DIM_> friend class Matrix;

		Matrix_RefProxy<T, DIM> * m_proxy;

		typedef T value_type;

		// NO allocator_type!
		typedef size_t size_type;
		typedef size_t difference_type;

		typedef GlobIter<value_type, DIM> iterator;
		typedef const GlobIter<value_type, DIM> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		typedef GlobRef<value_type> reference;
		typedef const GlobRef<value_type> const_reference;

		typedef GlobIter<value_type, DIM> pointer;
		typedef const GlobIter<value_type, DIM> const_pointer;

		operator Local_Ref<T, DIM, CUR - 1> && ()
		{
			Local_Ref<T, DIM, CUR - 1>  ref;
			ref.m_proxy = m_proxy;
			return std::move(ref);
		}

		operator Matrix_Ref<T, DIM, CUR> ()
		{
			Matrix_Ref<T, DIM, CUR>  ref;
			ref.m_proxy = m_proxy;
			return std::move(ref);
		}

		long long extent(size_t dim) const
		{
			assert(dim < DIM && dim >= 0);
			return m_proxy->m_mat->m_pattern.local_extent(dim);
		};

		size_type size() const
		{
			return m_proxy->m_viewspec.size();
		}

		Local_Ref<T, DIM, CUR>() = default;

		Local_Ref<T, DIM, CUR>(Matrix<T, DIM>* mat)
		{
			m_proxy = new Matrix_RefProxy < T, DIM > ;
			*m_proxy = *(mat->m_ref.m_proxy);

			for (int i = 0; i < DIM; i++)
			{
				m_proxy->m_viewspec.begin[i] = 0;
				m_proxy->m_viewspec.range[i] = mat->m_pattern.local_extent(i);
			}

			m_proxy->m_viewspec.update_size();
		}

		T& at_(size_type pos) {
			if (!(pos < m_proxy->m_viewspec.size()))
		{
				printf("pos %d size %d\n", pos, m_proxy->m_viewspec.size());
				throw std::out_of_range("Out of range");
		}
			return m_proxy->m_mat->lbegin()[pos];
		}

		template<typename ... Args>
		T& at(Args... args) {

			assert(sizeof...(Args) == DIM - m_proxy->m_dim);
			std::array<long long, DIM> coord = { args... };

			for(int i=m_proxy->m_dim;i<DIM;i++)
				m_proxy->m_coord[i] = coord[i-m_proxy->m_dim];

			return at_(m_proxy->m_mat->m_pattern.local_at_(m_proxy->m_coord, m_proxy->m_viewspec));

		}

		template<typename... Args>
		T& operator()(Args... args)
		{
			return at(args...);
		}

		Local_Ref<T, DIM, CUR - 1> && operator[](size_t n) && {

			Local_Ref<T, DIM, CUR - 1>  ref;
			ref.m_proxy = m_proxy;

			m_proxy->m_coord[m_proxy->m_dim] = n;
			m_proxy->m_dim++;

			m_proxy->m_viewspec.update_size();

			return std::move(ref);
		};

		Local_Ref<T, DIM, CUR - 1> operator[](size_t n) const & {
			Local_Ref<T, DIM, CUR - 1> ref;
			ref.m_proxy = new Matrix_RefProxy < T, DIM > ;
			ref.m_proxy->m_coord = m_proxy->m_coord;
			ref.m_proxy->m_coord[m_proxy->m_dim] = n;
			ref.m_proxy->m_dim = m_proxy->m_dim + 1;
			ref.m_proxy->m_mat = m_proxy->m_mat;
			ref.m_proxy->m_viewspec = m_proxy->m_viewspec;
			
			ref.m_proxy->m_viewspec.view_dim--;
			ref.m_proxy->m_viewspec.update_size();

			return ref;
		}

		template<size_t SUBDIM>
		Local_Ref<T, DIM, DIM - 1> sub(size_type n)
		{
			static_assert(DIM - 1 > 0, "Too low dim");
			static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong SUBDIM");

			size_t target_dim = SUBDIM + m_proxy->m_dim;

			Local_Ref<T, DIM, DIM - 1> ref;
			Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy < T, DIM > ;
			std::fill(proxy->m_coord.begin(), proxy->m_coord.end(), 0);

			ref.m_proxy = proxy;

			ref.m_proxy->m_coord[target_dim] = 0;

			ref.m_proxy->m_viewspec = m_proxy->m_viewspec;
			ref.m_proxy->m_viewspec.begin[target_dim] = n;
			ref.m_proxy->m_viewspec.range[target_dim] = 1;
			ref.m_proxy->m_viewspec.view_dim--;
			ref.m_proxy->m_viewspec.update_size();

			ref.m_proxy->m_mat = m_proxy->m_mat;
			ref.m_proxy->m_dim = m_proxy->m_dim + 1;

			return ref;
		}

		Local_Ref<T, DIM, DIM - 1> col(size_type n)
		{
			return sub<1>(n);
		}

		Local_Ref<T, DIM, DIM - 1> row(size_type n)
		{
			return sub<0>(n);
		}

		template<size_t SUBDIM>
		Local_Ref<T, DIM, DIM> submat(size_type n, size_type range)
		{
			static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong SUBDIM");
			Local_Ref<T, DIM, DIM> ref;
			Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy < T, DIM > ;
			std::fill(proxy->m_coord.begin(), proxy->m_coord.end(), 0);

			ref.m_proxy = proxy;
			ref.m_proxy->m_viewspec = m_proxy->m_viewspec;
			ref.m_proxy->m_viewspec.begin[SUBDIM] = n;
			ref.m_proxy->m_viewspec.range[SUBDIM] = range;
			ref.m_proxy->m_mat = m_proxy->m_mat;

			ref.m_proxy->m_viewspec.update_size();

			return ref;
		}

		Local_Ref<T, DIM, DIM> rows(size_type n, size_type range)
		{
			return submat<0>(n, range);
		}

		Local_Ref<T, DIM, DIM> cols(size_type n, size_type range)
		{
			return submat<1>(n, range);
		}
	};

	template <typename T, size_t DIM, size_t CUR = DIM> class Matrix_Ref {

	private:
	public:
		template<typename T_, size_t DIM_> friend class Matrix;
		Matrix_RefProxy<T, DIM> * m_proxy;

		typedef T value_type;

		// NO allocator_type!
		typedef size_t size_type;
		typedef size_t difference_type;

		typedef GlobIter<value_type, DIM> iterator;
		typedef const GlobIter<value_type, DIM> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		typedef GlobRef<value_type> reference;
		typedef const GlobRef<value_type> const_reference;

		typedef GlobIter<value_type, DIM> pointer;
		typedef const GlobIter<value_type, DIM> const_pointer;

		operator Matrix_Ref<T, DIM, CUR - 1> && ()
		{
			Matrix_Ref<T, DIM, CUR - 1> ref =  Matrix_Ref < T, DIM, CUR - 1 >() ;
			ref.m_proxy = m_proxy;
			return std::move(ref);
		}

		Matrix_Ref<T, DIM, CUR>() = default;

		Pattern<DIM> &pattern() {
			return m_proxy->m_mat->m_pattern;
		}

		Team &team() {
			return m_proxy->m_mat->m_team;
		}

		constexpr size_type size() const noexcept{
			return m_proxy->m_size;
		}

		size_type extent(size_type dim) const noexcept{
			return m_proxy->m_viewspec.range[dim];
		}

		constexpr bool empty() const noexcept{
			return size() == 0;
		}

		void barrier() const {
			m_proxy->m_mat->m_team.barrier();
		}

		void forall(std::function<void(long long)> func) {
			m_proxy->m_mat->m_pattern.forall(func, m_proxy->m_viewspec);
		}

		Matrix_Ref<T, DIM, CUR - 1> && operator[](size_t n) && {

			Matrix_Ref<T, DIM, CUR - 1> ref;

			m_proxy->m_coord[m_proxy->m_dim] = n;
			m_proxy->m_dim++;
			m_proxy->m_viewspec.update_size();

			ref.m_proxy = m_proxy;

			return std::move(ref);
		};

		Matrix_Ref<T, DIM, CUR - 1> operator[](size_t n) const & {

			Matrix_Ref<T, DIM, CUR - 1> ref;
			ref.m_proxy = new Matrix_RefProxy < T, DIM > ;
			ref.m_proxy->m_coord = m_proxy->m_coord;
			ref.m_proxy->m_coord[m_proxy->m_dim] = n;
			ref.m_proxy->m_dim = m_proxy->m_dim + 1;
			ref.m_proxy->m_mat = m_proxy->m_mat;
			ref.m_proxy->m_viewspec = m_proxy->m_viewspec;
			ref.m_proxy->m_viewspec.view_dim--;
			ref.m_proxy->m_viewspec.update_size();

			return ref;
		}

		template<size_t SUBDIM>
		Matrix_Ref<T, DIM, DIM - 1> sub(size_type n)
		{
			static_assert(DIM - 1 > 0, "Too low dim");
			static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong SUBDIM");

			size_t target_dim = SUBDIM + m_proxy->m_dim;

			Matrix_Ref<T, DIM, DIM - 1> ref;
			printf("new6\n");
			Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy < T, DIM > ;
			std::fill(proxy->m_coord.begin(), proxy->m_coord.end(), 0);

			ref.m_proxy = proxy;

			ref.m_proxy->m_coord[target_dim] = 0;

			ref.m_proxy->m_viewspec = m_proxy->m_viewspec;
			ref.m_proxy->m_viewspec.begin[target_dim] = n;
			ref.m_proxy->m_viewspec.range[target_dim] = 1;
			ref.m_proxy->m_viewspec.view_dim--;
			ref.m_proxy->m_viewspec.update_size();

			ref.m_proxy->m_mat = m_proxy->m_mat;
			ref.m_proxy->m_dim = m_proxy->m_dim + 1;

			return ref;
		}

		Matrix_Ref<T, DIM, DIM - 1> col(size_type n)
		{
			return sub<1>(n);
		}

		Matrix_Ref<T, DIM, DIM - 1> row(size_type n)
		{
			return sub<0>(n);
		}

		template<size_t SUBDIM>
		Matrix_Ref<T, DIM, DIM> submat(size_type n, size_type range)
		{
			static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong SUBDIM");
			Matrix_Ref<T, DIM, DIM> ref;
			Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy < T, DIM > ;
			std::fill(proxy->m_coord.begin(), proxy->m_coord.end(), 0);

			ref.m_proxy = proxy;
			ref.m_proxy->m_viewspec = m_proxy->m_viewspec;
			ref.m_proxy->m_viewspec.begin[SUBDIM] = n;
			ref.m_proxy->m_viewspec.range[SUBDIM] = range;
			ref.m_proxy->m_mat = m_proxy->m_mat;
			ref.m_proxy->m_viewspec.update_size();

			return ref;
		}

		Matrix_Ref<T, DIM, DIM> rows(size_type n, size_type range)
		{
			return submat<0>(n, range);
		}

		Matrix_Ref<T, DIM, DIM> cols(size_type n, size_type range)
		{
			return submat<1>(n, range);
		}

		reference at_(size_type unit , size_type elem) {
			if (!(elem < m_proxy->m_viewspec.size()))
				throw std::out_of_range("Out of range");

			return m_proxy->m_mat->begin().get(unit, elem);
		}

		template<typename ... Args>
		reference at(Args... args) {

			assert(sizeof...(Args) == DIM - m_proxy->m_dim);
			std::array<long long, DIM> coord = { args... };

			for(int i=m_proxy->m_dim;i<DIM;i++)
				m_proxy->m_coord[i] = coord[i-m_proxy->m_dim];
           
			size_t unit = m_proxy->m_mat->m_pattern.atunit_(m_proxy->m_coord, m_proxy->m_viewspec);
			size_t elem = m_proxy->m_mat->m_pattern.at_(m_proxy->m_coord, m_proxy->m_viewspec);
			return at_(unit , elem);

		}

		template<typename... Args>
		reference operator()(Args... args)
		{
			return at(args...);
		}

		Pattern<DIM> pattern() const {
			return m_proxy->m_mat->m_pattern;
		}

		//For 1D
		bool islocal(size_type n) {
			return m_proxy->m_mat->m_pattern.index_to_unit(n, m_proxy->m_viewspec) == m_proxy->m_mat->m_myid;
		}

		// For ND
		bool islocal(size_t dim, size_type n) {
			return m_proxy->m_mat->m_pattern.is_local(n, m_proxy->m_mat->m_myid, dim, m_proxy->m_viewspec);
		}

		template <int level> dash::HView<Matrix<T, DIM>, level, DIM> hview() {
			return dash::HView<Matrix<T, DIM>, level, DIM>(*this);
		}
	};

	template <typename T, size_t DIM>
	class Local_Ref < T, DIM, 0 >
	{

	private:

	public:
		template<typename T_, size_t DIM_> friend class Matrix;
		Matrix_RefProxy<T, DIM> * m_proxy;

		Local_Ref<T, DIM, 0>() = default;

		T* at_(size_t pos) {
			if (!(pos < m_proxy->m_mat->size()))
				throw std::out_of_range("Out of range");
			return &(m_proxy->m_mat->lbegin()[pos]);
		}

		operator T()
		{          
			T ret = *at_(m_proxy->m_mat->m_pattern.local_at_(m_proxy->m_coord, m_proxy->m_viewspec));
			delete m_proxy;
			return ret;
		}

		T operator=(const T value)
		{
			T* ref = at_(m_proxy->m_mat->m_pattern.local_at_(m_proxy->m_coord, m_proxy->m_viewspec));
			*ref = value;
			delete m_proxy;
			return value;
		}
	};

	template <typename T, size_t DIM>
	class Matrix_Ref < T, DIM, 0 >
	{

	private:

	public:
		template<typename T_, size_t DIM_> friend class Matrix;
		Matrix_RefProxy<T, DIM> * m_proxy;

		
		GlobRef<T> at_(size_t unit, size_t elem) const {
			return m_proxy->m_mat->begin().get(unit, elem);

		}

		Matrix_Ref<T, DIM, 0>() = default;

		operator T()
		{
			GlobRef<T> ref = at_(m_proxy->m_mat->m_pattern.atunit_(m_proxy->m_coord, m_proxy->m_viewspec), m_proxy->m_mat->m_pattern.at_(m_proxy->m_coord, m_proxy->m_viewspec));
			delete m_proxy;	
			return ref;
		}

		T operator=(const T value)
		{
			GlobRef<T> ref = at_(m_proxy->m_mat->m_pattern.atunit_(m_proxy->m_coord, m_proxy->m_viewspec), m_proxy->m_mat->m_pattern.at_(m_proxy->m_coord, m_proxy->m_viewspec));
			ref = value;
			delete m_proxy;
			return value;
		}
	};


	template <typename ELEMENT_TYPE, size_t DIM> class Matrix {

	public:
		typedef ELEMENT_TYPE value_type;

		// NO allocator_type!
		typedef size_t size_type;
		typedef size_t difference_type;

		typedef GlobIter<value_type, DIM> iterator;
		typedef const GlobIter<value_type, DIM> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		typedef GlobRef<value_type> reference;
		typedef const GlobRef<value_type> const_reference;

		typedef GlobIter<value_type, DIM> pointer;
		typedef const GlobIter<value_type, DIM> const_pointer;

	private:
		dash::Team &m_team;
		dart_unit_t m_myid;
		dash::Pattern<DIM> m_pattern;
		size_type m_size;  // total size (#elements)
		size_type m_lsize; // local size (#local elements)
		pointer *m_ptr;
		dart_gptr_t m_dart_gptr;
		Matrix_Ref<ELEMENT_TYPE, DIM, DIM> m_ref;

	public:

		template<typename T_, size_t DIM1, size_t DIM2> friend class Matrix_Ref;
		template<typename T_, size_t DIM1, size_t DIM2> friend class Local_Ref;

		Local_Ref<ELEMENT_TYPE, DIM, DIM> m_local;

		static_assert(std::is_trivial<ELEMENT_TYPE>::value,
			"Element type must be trivial copyable");

	public:

		Local_Ref<ELEMENT_TYPE, DIM, DIM> local() const
		{
			return m_local;
		}

		Matrix(const dash::SizeSpec<DIM> &ss,
			const dash::DistSpec<DIM> &ds = dash::DistSpec<DIM>(),
			Team &t = dash::Team::All(), const TeamSpec<DIM> &ts = TeamSpec<DIM>())
			: m_team(t), m_pattern(ss, ds, ts, t) {
			// Matrix is a friend of class Team
			dart_team_t teamid = m_team.m_dartid;

			size_t lelem = m_pattern.max_elem_per_unit();
			size_t lsize = lelem * sizeof(value_type);

			m_dart_gptr = DART_GPTR_NULL;
			dart_ret_t ret = dart_team_memalloc_aligned(teamid, lsize, &m_dart_gptr);

			m_ptr = new GlobIter<value_type, DIM>(m_pattern, m_dart_gptr, 0);

			m_size = m_pattern.nelem();
			m_lsize = lelem;

			m_myid = m_team.myid();

			m_ref.m_proxy = new Matrix_RefProxy < value_type, DIM > ;
			m_ref.m_proxy->m_dim = 0;
			m_ref.m_proxy->m_mat = this;
			m_ref.m_proxy->m_viewspec = m_pattern.m_viewspec;
			m_local = Local_Ref<ELEMENT_TYPE, DIM, DIM>(this);
		}

		// delegating constructor
		Matrix(const dash::Pattern<DIM> &pat)
			: Matrix(pat.sizespec(), pat.distspec(), pat.team(), pat.teamspec()) {}

		// delegating constructor
		Matrix(size_t nelem, Team &t = dash::Team::All())
			: Matrix(dash::Pattern<DIM>(nelem, t)) {}

		~Matrix() {
			dart_team_t teamid = m_team.m_dartid;
			dart_team_memfree(teamid, m_dart_gptr);
		}

		Pattern<DIM> &pattern() {
			return m_pattern;
		}

		Team &team() {
			return m_team;
		}

		constexpr size_type size() const noexcept{
			return m_size;
		}

		size_type extent(size_type dim) const noexcept{
			return m_ref.m_proxy->m_viewspec.range[dim];
		}

		constexpr bool empty() const noexcept{
			return size() == 0;
		}

		void barrier() const {
			m_team.barrier();
		}

		const_pointer data() const noexcept{
			return *m_ptr;
		}

		iterator begin() noexcept{ return iterator(data()); }

		iterator end() noexcept{ return iterator(data() + m_size); }

		ELEMENT_TYPE *lbegin() noexcept{
			void *addr;
			dart_gptr_t gptr = m_dart_gptr;

			dart_gptr_setunit(&gptr, m_myid);
			dart_gptr_getaddr(gptr, &addr);
			return (ELEMENT_TYPE *)(addr);
		}

		ELEMENT_TYPE *lend() noexcept{
			void *addr;
			dart_gptr_t gptr = m_dart_gptr;

			dart_gptr_setunit(&gptr, m_myid);
			dart_gptr_incaddr(&gptr, m_lsize * sizeof(ELEMENT_TYPE));
			dart_gptr_getaddr(gptr, &addr);
			return (ELEMENT_TYPE *)(addr);
		}

		void forall(std::function<void(long long)> func) {
			m_pattern.forall(func);
		}

#if 0
		iterator lbegin() noexcept
		{
			// xxx needs fix
			return iterator(data() + m_team.myid()*m_lsize);
		}

		iterator lend() noexcept
		{
			// xxx needs fix
			size_type end = (m_team.myid() + 1)*m_lsize;
			if (m_size < end) end = m_size;

			return iterator(data() + end);
		}
#endif

		template<size_t SUBDIM>
		Matrix_Ref<ELEMENT_TYPE, DIM, DIM - 1> sub(size_type n)
		{
			return m_ref.sub<SUBDIM>(n);
		}

		Matrix_Ref<ELEMENT_TYPE, DIM, DIM - 1> col(size_type n)
		{
			return m_ref.sub<1>(n);
		}

		Matrix_Ref<ELEMENT_TYPE, DIM, DIM - 1> row(size_type n)
		{
			return m_ref.sub<0>(n);
		}

		template<size_t SUBDIM>
		Matrix_Ref<ELEMENT_TYPE, DIM, DIM> submat(size_type n, size_type range)
		{
			return m_ref.submat<SUBDIM>(n, range);
		}

		Matrix_Ref<ELEMENT_TYPE, DIM, DIM> rows(size_type n, size_type range)
		{
			return m_ref.submat<0>(n, range);
		}

		Matrix_Ref<ELEMENT_TYPE, DIM, DIM> cols(size_type n, size_type range)
		{
			return m_ref.submat<1>(n, range);
		}

		Matrix_Ref<ELEMENT_TYPE, DIM, DIM - 1> operator[](size_type n)
		{
			return m_ref.operator[](n);
		}


		template<typename ... Args>
		reference at(Args... args) {
			return m_ref.at(args...);
		}

		template<typename... Args>
		reference operator()(Args... args)
		{
			return m_ref.at(args...);
		}

		Pattern<DIM> pattern() const {
			return m_pattern;
		}

		//For 1D
		bool islocal(size_type n) {
			return m_ref.islocal(n);
		}

		// For ND
		bool islocal(size_t dim, size_type n) {
			return m_ref.islocal(dim, n);
		}

		template <int level> dash::HView<Matrix<ELEMENT_TYPE, DIM>, level, DIM> hview() {
			return m_ref.hview<level>();
		}

		operator Matrix_Ref<ELEMENT_TYPE, DIM, DIM> ()
		{
			return m_ref;
		}
	};

}
#endif
