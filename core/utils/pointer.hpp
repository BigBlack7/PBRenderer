#pragma once
#include "debugMacro.hpp"
#include <type_traits>
#include <cassert>
#include <cstdint>

namespace pbrt
{
    template <size_t MAX_TS, typename CurrentT, typename... Ts>
    struct Indices
    {
        static constexpr size_t __idx__ = Indices<MAX_TS, Ts...>::__idx__ - 1;
    };

    template <size_t MAX_TS, typename T>
    struct Indices<MAX_TS, T>
    {
        static constexpr size_t __idx__ = MAX_TS - 1;
    };

    template <size_t MAX_TS, typename Compare, typename T, typename... Ts>
    struct GetIndexOf
    {
    private:
        static constexpr size_t GetIndex()
        {
            if constexpr (std::is_same_v<Compare, T>)
            {
                return Indices<MAX_TS, T, Ts...>::__idx__;
            }
            else if constexpr (sizeof...(Ts) == 0)
            {
                static_assert(sizeof...(Ts) != 0, "Compare not in Ts...");
            }
            else
            {
                return GetIndexOf<MAX_TS, Compare, Ts...>::__idx__;
            }
        }

    public:
        static constexpr size_t __idx__ = GetIndex();
    };

    template <size_t Idx, typename T, typename... Ts>
    struct GetTypeOf
    {
    private:
        static constexpr auto *GetTypePointer()
        {
            if constexpr (Idx == 0)
            {
                return static_cast<T *>(nullptr);
            }
            else
            {
                return static_cast<typename GetTypeOf<Idx - 1, Ts...>::type *>(nullptr);
            }
        }

    public:
        using type = std::remove_pointer_t<decltype(GetTypePointer())>;
    };

    class CombinedPointer
    {
    public:
        CombinedPointer(uintptr_t ptr, uint8_t tag, bool is_const) : mPtr(ptr), mTag(tag), mIsConst(is_const) {}
        uintptr_t GetPtr() const { return mPtr; }
        uint8_t GetTag() const { return mTag; }
        bool IsConst() const { return mIsConst; }
        void SetPtr(uintptr_t ptr) { this->mPtr = ptr; }
        void SetTag(uint8_t tag) { this->mTag = tag; }
        void SetConst(bool is_const) { this->mIsConst = is_const; }

    private:
        uintptr_t mPtr;
        bool mIsConst;
        uint8_t mTag;
    };

    class TaggedPointer
    {
    public:
        TaggedPointer(uintptr_t ptr, uint8_t tag, bool is_const)
        {
            DEBUG_INFO(assert((ptr & mConstTagMask) == 0))
            mBits = (uintptr_t(is_const) << mConstShift) | (uintptr_t(tag) << mTagShift) | ptr;
        }
        uintptr_t GetPtr() const { return mBits & mPtrMask; }
        uint8_t GetTag() const { return (mBits & mTagMask) >> mTagShift; };
        bool IsConst() const { return (mBits & mConstMask) != 0; }
        void SetPtr(uintptr_t ptr) { mBits = (mBits & mConstTagMask) | ptr; }
        void SetTag(uint8_t tag) { mBits = (mBits & mPtrMask) | ((uintptr_t(tag) << mTagShift) & mTagMask); }
        void SetConst(bool is_const) { mBits = (mBits & mTagPtrMask) | (uintptr_t(is_const) << mConstShift); }

    private:
        uintptr_t mBits;
        static constexpr uintptr_t mConstShift = sizeof(uintptr_t) * uintptr_t(8) - uintptr_t(1);
        static constexpr uintptr_t mConstMask = uintptr_t(1) << mConstShift;
        static constexpr uintptr_t mTagBitNum = 6;
        static constexpr uintptr_t mTagShift = mConstShift - mTagBitNum;
        static constexpr uintptr_t mTagMask = ((uintptr_t(1) << mTagBitNum) - uintptr_t(1)) << mTagShift;
        static constexpr uintptr_t mConstTagMask = mTagMask | mConstMask;
        static constexpr uintptr_t mPtrMask = ~mConstTagMask;
        static constexpr uintptr_t mTagPtrMask = ~mConstMask;
    };

    namespace internal
    {
        template <typename F, typename T0>
        auto Dispatch(F &&func, void *ptr, uint8_t tag)
        {
            return func(static_cast<T0 *>(ptr));
        }

        template <typename F, typename T0, typename T1>
        auto Dispatch(F &&func, void *ptr, uint8_t tag)
        {
            switch (tag)
            {
            case 0:
                return func(static_cast<T0 *>(ptr));
            default:
                return func(static_cast<T1 *>(ptr));
            }
        }

        template <typename F, typename T0, typename T1, typename T2>
        auto Dispatch(F &&func, void *ptr, uint8_t tag)
        {
            switch (tag)
            {
            case 0:
                return func(static_cast<T0 *>(ptr));
            case 1:
                return func(static_cast<T1 *>(ptr));
            default:
                return func(static_cast<T2 *>(ptr));
            }
        }

        template <typename F, typename T0, typename T1, typename T2, typename T3>
        auto Dispatch(F &&func, void *ptr, uint8_t tag)
        {
            switch (tag)
            {
            case 0:
                return func(static_cast<T0 *>(ptr));
            case 1:
                return func(static_cast<T1 *>(ptr));
            case 2:
                return func(static_cast<T2 *>(ptr));
            default:
                return func(static_cast<T3 *>(ptr));
            }
        }

        template <typename F, typename T0, typename T1, typename T2, typename T3, typename T4>
        auto Dispatch(F &&func, void *ptr, uint8_t tag)
        {
            switch (tag)
            {
            case 0:
                return func(static_cast<T0 *>(ptr));
            case 1:
                return func(static_cast<T1 *>(ptr));
            case 2:
                return func(static_cast<T2 *>(ptr));
            case 3:
                return func(static_cast<T3 *>(ptr));
            default:
                return func(static_cast<T4 *>(ptr));
            }
        }

        template <typename F, typename T0>
        auto DispatchConst(F &&func, const void *ptr, uint8_t tag)
        {
            return func(static_cast<const T0 *>(ptr));
        }

        template <typename F, typename T0, typename T1>
        auto DispatchConst(F &&func, const void *ptr, uint8_t tag)
        {
            switch (tag)
            {
            case 0:
                return func(static_cast<const T0 *>(ptr));
            default:
                return func(static_cast<const T1 *>(ptr));
            }
        }

        template <typename F, typename T0, typename T1, typename T2>
        auto DispatchConst(F &&func, const void *ptr, uint8_t tag)
        {
            switch (tag)
            {
            case 0:
                return func(static_cast<const T0 *>(ptr));
            case 1:
                return func(static_cast<const T1 *>(ptr));
            default:
                return func(static_cast<const T2 *>(ptr));
            }
        }

        template <typename F, typename T0, typename T1, typename T2, typename T3>
        auto DispatchConst(F &&func, const void *ptr, uint8_t tag)
        {
            switch (tag)
            {
            case 0:
                return func(static_cast<const T0 *>(ptr));
            case 1:
                return func(static_cast<const T1 *>(ptr));
            case 2:
                return func(static_cast<const T2 *>(ptr));
            default:
                return func(static_cast<const T3 *>(ptr));
            }
        }

        template <typename F, typename T0, typename T1, typename T2, typename T3, typename T4>
        auto DispatchConst(F &&func, const void *ptr, uint8_t tag)
        {
            switch (tag)
            {
            case 0:
                return func(static_cast<const T0 *>(ptr));
            case 1:
                return func(static_cast<const T1 *>(ptr));
            case 2:
                return func(static_cast<const T2 *>(ptr));
            case 3:
                return func(static_cast<const T3 *>(ptr));
            default:
                return func(static_cast<const T4 *>(ptr));
            }
        }
    }

    template <typename... Ts>
    class GeneralizedPtr
    {
    private:
        static constexpr size_t MAX_TS = sizeof...(Ts);
        TaggedPointer mPointer;

    public:
        template <typename T>
        GeneralizedPtr(T *ptr) : mPointer(reinterpret_cast<uintptr_t>(ptr), GetIndexOf<MAX_TS, std::decay_t<T>, Ts...>::__idx__, std::is_const_v<T>) {}

        GeneralizedPtr() : mPointer(0, MAX_TS, true) {}

        bool IsValid() const { return mPointer.GetPtr() != 0; }

        uintptr_t GetPtr() const { return mPointer.GetPtr(); }

    private:
        template <size_t Idx>
        typename GetTypeOf<Idx, Ts...>::type *Cast()
        {
            // Idx must equal to Ts_idx
            return reinterpret_cast<typename GetTypeOf<Idx, Ts...>::type *>(mPointer.GetPtr());
        }

        template <size_t Idx>
        const auto *ConstCast() const
        {
            return reinterpret_cast<const typename GetTypeOf<Idx, Ts...>::type *>(mPointer.GetPtr());
        }

    protected:
        template <typename Func>
        auto Dispatch(Func &&func)
        {
            DEBUG_LINE(assert(!mPointer.IsConst()))
            return internal::Dispatch<Func, Ts...>(std::move(func), reinterpret_cast<void *>(mPointer.GetPtr()), mPointer.GetTag());
        }

        template <typename Func>
        auto DispatchConst(Func &&func) const
        {
            return internal::DispatchConst<Func, Ts...>(std::move(func), reinterpret_cast<const void *>(mPointer.GetPtr()), mPointer.GetTag());
        }
    };

#define DISPATCH(func_name, ...) Dispatch([&](auto *ptr) { return ptr->func_name(__VA_ARGS__); })
#define DISPATCH_CONST(func_name, ...) DispatchConst([&](const auto *ptr) { return ptr->func_name(__VA_ARGS__); })
}