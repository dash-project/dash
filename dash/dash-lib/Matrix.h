
#ifndef DASH_MATRIX_H_INCLUDED
#define DASH_MATRIX_H_INCLUDED

#include <type_traits>
#include <stdexcept>

#include "Team.h"
//#include "Pattern.h"
#include "Pattern.h"
#include "GlobPtr.h"
#include "GlobRef.h"
#include "HView.h"

#include "dart.h"

namespace dash {

template <typename T, size_t DIM> class Matrix;

template <typename T, size_t DIM, size_t CUR> class Matrix_Ref;
template <typename T, size_t DIM, size_t CUR> class Local_Ref;

template <typename T, size_t DIM> class LocalProxyMatrix {
private:
    Matrix<T, DIM> *m_ptr;

public:
    LocalProxyMatrix(Matrix<T, DIM> *ptr) {
        m_ptr = ptr;
    }

    T *begin() noexcept { return m_ptr->lbegin(); }

    T *end() noexcept { return m_ptr->lend(); }
};

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

template <typename T, size_t DIM, size_t CUR> class Local_Ref {

private:
	Matrix_RefProxy<T, DIM> * m_proxy;

public:
	template<typename T_, size_t DIM_> friend class Matrix;


	typedef T value_type;

	// NO allocator_type!
	typedef size_t size_type;
	typedef size_t difference_type;

	typedef GlobPtr<value_type, DIM> iterator;
	typedef const GlobPtr<value_type, DIM> const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	typedef GlobRef<value_type> reference;
	typedef const GlobRef<value_type> const_reference;

	typedef GlobPtr<value_type, DIM> pointer;
	typedef const GlobPtr<value_type, DIM> const_pointer;


	operator Matrix_Ref<T, DIM, CUR> && ()
	{
		Matrix_Ref<T, DIM, CUR> * ref = new Matrix_Ref<T, DIM, CUR>;
		ref->m_proxy = m_proxy;
		return std::move(*ref);
	}

	long long size()
	{
		return m_proxy->m_viewspec.size();
	}

	Local_Ref<T, DIM, CUR>() = default;

	Local_Ref<T, DIM, CUR>(Matrix<T, DIM>* mat)
	{
		Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy<T, DIM>;
		*proxy = *(mat->m_ref.m_proxy);
	}

	reference at_(size_type pos) {
		if (!(pos < m_proxy->m_mat->size()))
			throw std::out_of_range("Out of range");
		return m_proxy->m_mat->lbegin()[pos];
	}

	template<typename ... Args>
	reference at(Args... args) {

		static_assert(sizeof...(Args) == DIM, "Wrong parameter number");
		std::array<long long, DIM> coord = { args... };
		return at_(m_proxy->m_mat->m_pattern.local_at_(coord, m_proxy->m_viewspec));
	}
	
	Local_Ref<T, DIM, CUR - 1> && operator[](size_t n) && {

		m_proxy->m_coord[m_proxy->m_dim] = n;
		m_proxy->m_dim++;
		m_proxy->m_viewspec.view_dim--;
		return *this;
	};

	Local_Ref<T, DIM, CUR - 1> operator[](size_t n) const & {

		Local_Ref<T, DIM, CUR - 1> * ref = new Matrix_Ref<T, DIM, CUR - 1>;
		ref->m_proxy = new Matrix_RefProxy<T, DIM>;
		ref->m_proxy->m_coord = m_proxy->m_coord;
		ref->m_proxy->m_coord[m_proxy->m_dim] = n;
		ref->m_proxy->m_dim = m_proxy->m_dim + 1;
		ref->m_proxy->m_mat = m_proxy->mat;
		ref->m_proxy->m_viewspec = m_proxy->m_viewspec;
		ref->m_proxy->m_viewspec.view_dim--;

		return *ref;
	}

	template<size_t SUBDIM>
	Local_Ref<T, DIM, DIM - 1> sub(size_type n)
	{
		static_assert(DIM - 1 > 0, "Too low dim");
		static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong SUBDIM");

		Local_Ref<T, DIM, DIM - 1> ref;
		Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy<T, DIM>;
		std::fill(proxy->m_coord.begin(), proxy->m_coord.end(), -1);
		ref.m_proxy = proxy;
		ref.m_proxy->m_coord[SUBDIM] = n;
		ref.m_proxy->m_mat = m_proxy->m_mat;
		ref.m_proxy->m_viewspec.view_dim--;
		ref.m_proxy->m_dim++;

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
		Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy<T, DIM>;
		std::fill(proxy->m_coord.begin(), proxy->m_coord.end(), -1);

		ref.m_proxy = proxy;
		ref.m_proxy->m_viewspec.begin[SUBDIM] = n;
		ref.m_proxy->m_viewspec.range[SUBDIM] = range;
		ref.m_proxy->m_mat = m_proxy->m_mat;

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

template <typename T, size_t DIM, size_t CUR> class Matrix_Ref {

private:



public:
    template<typename T_, size_t DIM_> friend class Matrix;
    Matrix_RefProxy<T, DIM> * m_proxy;

	typedef T value_type;

	// NO allocator_type!
	typedef size_t size_type;
	typedef size_t difference_type;

	typedef GlobPtr<value_type, DIM> iterator;
	typedef const GlobPtr<value_type, DIM> const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	typedef GlobRef<value_type> reference;
	typedef const GlobRef<value_type> const_reference;

	typedef GlobPtr<value_type, DIM> pointer;
	typedef const GlobPtr<value_type, DIM> const_pointer;

    operator Matrix_Ref<T, DIM, CUR - 1> && ()
    {
        Matrix_Ref<T, DIM, CUR - 1> * ref = new Matrix_Ref<T, DIM, CUR - 1>;
        ref->m_proxy = m_proxy;
        return std::move(*ref);
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

	constexpr bool empty() const noexcept{
		return size() == 0;
	}

	void barrier() const {
		m_proxy->m_mat->m_team.barrier();
	}

	/*		const_pointer data() const noexcept{
		return *m_proxy->m_mat->m_ptr;
	}

//	iterator begin() noexcept{ return iterator(m_proxy->m_mat->data()); }

//	iterator end() noexcept{ return iterator(m_proxy->m_mat->data() + m_size); }

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
		}*/ 

	void forall(std::function<void(long long)> func) {
		m_proxy->m_mat->m_pattern.forall(func, m_proxy->m_viewspec);
	}

    Matrix_Ref<T, DIM, CUR - 1> && operator[](size_t n) && {

        m_proxy->m_coord[ m_proxy->m_dim] = n;
        m_proxy->m_dim++;
		m_proxy->m_viewspec.view_dim--;
        return *this;
    };

    Matrix_Ref<T, DIM, CUR - 1> operator[](size_t n) const & {

        Matrix_Ref<T, DIM, CUR - 1> * ref = new Matrix_Ref<T, DIM, CUR - 1>;
        ref->m_proxy = new Matrix_RefProxy<T, DIM>;
        ref->m_proxy->m_coord = m_proxy->m_coord;
        ref->m_proxy->m_coord[m_proxy->m_dim] = n;
        ref->m_proxy->m_dim = m_proxy->m_dim+1;
        ref->m_proxy->m_mat = m_proxy->mat;
		ref->m_proxy->m_viewspec = m_proxy->m_viewspec;
		ref->m_proxy->m_viewspec.view_dim--;

        return *ref;
    }

	template<size_t SUBDIM>
	Matrix_Ref<T, DIM, DIM - 1> sub(size_type n)
	{
		static_assert(DIM - 1 > 0, "Too low dim");
		static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong SUBDIM");

		Matrix_Ref<T, DIM, DIM - 1> ref;
		Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy<T, DIM>;
		std::fill(proxy->m_coord.begin(), proxy->m_coord.end(), -1);
		ref.m_proxy = proxy;
		ref.m_proxy->m_coord[SUBDIM] = n;
		ref.m_proxy->m_mat = m_proxy->m_mat;
		ref.m_proxy->m_viewspec.view_dim--;
		ref.m_proxy->m_dim++;

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
		Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy<T, DIM>;
		std::fill(proxy->m_coord.begin(), proxy->m_coord.end(), -1);

		ref.m_proxy = proxy;
		ref.m_proxy->m_viewspec.begin[SUBDIM] = n;
		ref.m_proxy->m_viewspec.range[SUBDIM] = range;
		ref.m_proxy->m_mat = m_proxy->m_mat;

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

	reference at_(size_type pos) {
		if (!(pos < m_proxy->m_mat->size()))
			throw std::out_of_range("Out of range");
		return m_proxy->m_mat->begin()[pos];
	}

	template<typename ... Args>
	reference at(Args... args) {

		static_assert(sizeof...(Args) == DIM, "Wrong parameter number");
		std::array<long long, DIM> coord = { args... };
		return at_(m_proxy->m_mat->m_pattern.index_to_elem(coord, m_proxy->m_viewspec));

	}

	template<typename... Args>
	reference operator()(Args... args)
	{
		static_assert(sizeof...(args) == DIM, "Wrong parameter number");
		std::array<long long, DIM> coord = { args... };
		return at_(m_proxy->m_mat->m_pattern.index_to_elem(coord, m_proxy->m_viewspec));
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
class Matrix_Ref<T, DIM, 0>
{

private:

public:
    template<typename T_, size_t DIM_> friend class Matrix;
    Matrix_RefProxy<T, DIM> * m_proxy;

    GlobRef<T> get_from_ref(std::array<long long, DIM> coord) const {

        return m_proxy->m_mat->at_(m_proxy->m_mat->m_pattern.index_to_elem(coord, m_proxy->m_viewspec));

    }

    Matrix_Ref<T, DIM, 0>()=default;

	operator T()
	{
		return get_from_ref(m_proxy->m_coord);
	}
	
	T operator=(const T value)
	{
		GlobRef<T> ref = get_from_ref(m_proxy->m_coord);
		ref = value;
		return value;
	}
};


template <typename ELEMENT_TYPE, size_t DIM> class Matrix {

public:
    typedef ELEMENT_TYPE value_type;

    // NO allocator_type!
    typedef size_t size_type;
    typedef size_t difference_type;

    typedef GlobPtr<value_type, DIM> iterator;
    typedef const GlobPtr<value_type, DIM> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef GlobRef<value_type> reference;
    typedef const GlobRef<value_type> const_reference;

    typedef GlobPtr<value_type, DIM> pointer;
    typedef const GlobPtr<value_type, DIM> const_pointer;

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

    Local_Ref<ELEMENT_TYPE, DIM, DIM> local;

    static_assert(std::is_trivial<ELEMENT_TYPE>::value,
                  "Element type must be trivial copyable");

public:

    Matrix(const dash::SizeSpec<DIM> &ss,
           const dash::DistSpec<DIM> &ds = dash::DistSpec<DIM>(),
           Team &t = dash::Team::All(), const TeamSpec<DIM> &ts = TeamSpec<DIM>())
        : m_team(t), m_pattern(ss, ds, ts, t), local(this) {
        // Matrix is a friend of class Team
        dart_team_t teamid = m_team.m_dartid;

        size_t lelem = m_pattern.max_elem_per_unit();
        size_t lsize = lelem * sizeof(value_type);

        m_dart_gptr = DART_GPTR_NULL;
        dart_ret_t ret = dart_team_memalloc_aligned(teamid, lsize, &m_dart_gptr);

        m_ptr = new GlobPtr<value_type, DIM>(m_pattern, m_dart_gptr, 0);

        m_size = m_pattern.nelem();
        m_lsize = lelem;

        m_myid = m_team.myid();

        // m_realsize = lelem * m_team.size();
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

    constexpr size_type size() const noexcept {
        return m_size;
    }

    constexpr bool empty() const noexcept {
        return size() == 0;
    }

    void barrier() const {
        m_team.barrier();
    }

    const_pointer data() const noexcept {
        return *m_ptr;
    }

    iterator begin() noexcept { return iterator(data()); }

    iterator end() noexcept { return iterator(data() + m_size); }

    ELEMENT_TYPE *lbegin() noexcept {
        void *addr;
        dart_gptr_t gptr = m_dart_gptr;

        dart_gptr_setunit(&gptr, m_myid);
        dart_gptr_getaddr(gptr, &addr);
        return (ELEMENT_TYPE *)(addr);
    }

    ELEMENT_TYPE *lend() noexcept {
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
		return m_ref.sub<0>(n);
    }

    reference at_(size_type pos) {
        if (!(pos < size()))
            throw std::out_of_range("Out of range");		
		return begin()[pos];
    }
	
	template<typename ... Args>
	reference at(Args... args) {
	
		static_assert(sizeof...(Args) == DIM, "Wrong parameter number");
		std::array<long long, DIM> coord = { args... };
		return at_(m_pattern.index_to_elem(coord, m_ref.m_proxy->m_viewspec));
		
	}
	
	template<typename... Args> 
	reference operator()(Args... args)
	{
		static_assert(sizeof...(args) == DIM, "Wrong parameter number");
		std::array<long long, DIM> coord = { args... };
		return at_(m_pattern.index_to_elem(coord, m_ref.m_proxy->m_viewspec));
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
};

}
#endif
