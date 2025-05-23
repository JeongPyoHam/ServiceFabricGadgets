#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Common.h"

#include "TestTools.h"
#include "TestAsyncOperation.h"

using namespace std;
using namespace Common;

namespace LifeTest
{

// I'm not creative. class name is 'Abc'. Ha Ha Ha
class Abc
{
public:
    Abc(std::wstring const & id)
        : Id(id)
    {
        PrintCurrentThreadId(L"ctor: " + id);
    }

    virtual ~Abc()
    {
        PrintCurrentThreadId(L"dtor: " + Id);
    }

    std::wstring const Id;
};

class NeverDieAsyncOperation : public Common::AsyncOperation
{
public:
    NeverDieAsyncOperation(AsyncCallback const & callback, AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
    {
        PrintCurrentThreadId(L"NeverDieAsyncOperation ctor");
    }

    virtual ~NeverDieAsyncOperation()
    {
        PrintCurrentThreadId(L"NeverDieAsyncOperation dtor");
    }

protected:
    void OnStart(AsyncOperationSPtr const& thisSPtr) override
    {
        for (int i = 0; i < 3; ++i)
        {
            StartChildAsyncOp(thisSPtr);
        }
    }

    void StartChildAsyncOp(AsyncOperationSPtr const & thisSPtr)
    {
        auto op = AsyncOperation::CreateAndStart<TestCommon::TestAsyncOperation>(
            [this](AsyncOperationSPtr const & operation) {
                this->OnCompleted(operation);
            },
            thisSPtr);

        childrentAsyncOps_.emplace_back(op);
    }

    void OnCompleted(AsyncOperationSPtr const& operation)
    {
        auto error = TestCommon::TestAsyncOperation::End(operation);
        if (!error.IsSuccess())
        {
            // trace
        }

        auto thisSPtr = operation->Parent;

        ++completedCount_;

        if (completedCount_.load() == 3)
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }
    }

    std::atomic<int> completedCount_;
    std::vector<AsyncOperationSPtr> childrentAsyncOps_;
};

// RootedObject sample class
class TestRootedObject
    : public Common::RootedObject
{
public:
    TestRootedObject(wstring const & id, Common::ComponentRoot const & root)
        : RootedObject(root)
        , id_(id)
    {
        PrintCurrentThreadId(L"TestRootedObject ctor: " + id_);
    }

    ~TestRootedObject()
    {
        PrintCurrentThreadId(L"TestRootedObject dtor: " + id_);
    }

    void OnMessage(wstring const & message)
    {
        AsyncOperationSPtr asyncOp = AsyncOperation::CreateAndStart<InnerAsyncOperation>(

            // Will "this" have been destructed, when callback is gonna run?
            [message, this](AsyncOperationSPtr const& op) {
                this->OnInnerCompleted(op, message);
            },

            // No. this is RootedObject, which is rooted to Root.
            // CreateAsyncOperationRoot increments ref to Root, that will be alive during this async op
            // So, the rooted object (this) is alive, too.
            Root.CreateAsyncOperationRoot());
    }

private:
    class InnerAsyncOperation : public AsyncOperation
    {
    public:
        InnerAsyncOperation(AsyncCallback const & callback, AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent)
        { }

        ~InnerAsyncOperation() {}

    protected:
        void OnStart(AsyncOperationSPtr const& thisSPtr)
        {
            Threadpool::Post(
                [thisSPtr, this]() {
                    Sleep(3000);
                    this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                });
        }
    };

    void OnInnerCompleted(AsyncOperationSPtr const & operation, wstring const & message)
    {
        auto error = AsyncOperation::End(operation);

        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidOperation));

        PrintCurrentThreadId(L"OnInnerCompleted, messageId: " + message);
    }

    wstring const id_;
};

///////////////////
// pure virtual class for test
// 
// imitate the interface
class ITestInterface
{
public:
    virtual AsyncOperationSPtr BeginHelloWorld(AsyncCallback const & callback, AsyncOperationSPtr const & parent) = 0;
    virtual ErrorCode EndHelloWorld(AsyncOperationSPtr const&) = 0;
};

class TestFabricComponent
    : public Common::FabricComponent
    , public Common::ComponentRoot
    // this component supports ITestInterface
    , public ITestInterface
{
public:
    TestFabricComponent()
        : FabricComponent()
        , ComponentRoot()
        , member_obj(L"member", * this)
        , unique_ptr_obj(make_unique<TestRootedObject>(L"unique_ptr", *this))
    {
        PrintCurrentThreadId(L"TestFabricComponent ctor");
    }

    ~TestFabricComponent()
    {
        PrintCurrentThreadId(L"TestFabricComponent dtor");
    }

protected:
    virtual Common::ErrorCode OnOpen() override
    {
        PrintCurrentThreadId(L"TestFabricComponent::OnOpen");

        shared_ptr_obj = make_shared<TestRootedObject>(L"shared_ptr", *this);

        return ErrorCodeValue::Success;
    }

    virtual Common::ErrorCode OnClose() override
    {
        PrintCurrentThreadId(L"TestFabricComponent::OnClose");

        shared_ptr_obj.reset();

        return ErrorCodeValue::Success;
    }

    virtual void OnAbort() override
    {
        PrintCurrentThreadId(L"TestFabricComponent::OnAbort");
    }

    /////////////////////
    // ITestInterface implementation
    /////////////////////
public:
    AsyncOperationSPtr BeginHelloWorld(AsyncCallback const& callback, AsyncOperationSPtr const& parent) override
    {
        if (!this->IsOpened())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::ObjectClosed, callback, parent);
        }
        else
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::Success, callback, parent);
        }
    }
    ErrorCode EndHelloWorld(AsyncOperationSPtr const& operation) override
    {
        return CompletedAsyncOperation::End(operation);
    }

public:
    TestRootedObject member_obj;
    std::unique_ptr<TestRootedObject> unique_ptr_obj;

    ////////////////////
    // shared_ptr with RootedObject. Isn't it weird?
    ///////////////////
    std::shared_ptr<TestRootedObject> shared_ptr_obj;
};

//
// The naming scheme uses postfix 'SPtr' for shared_ptr.
// 'UPtr' for unique_ptr. 'Ptr' for bare pointer.
// They are defined by 'typedef' or 'using'
//
// For example,
// 
//   using AbcSPtr = std::shared_ptr<Abc>;
//   typedef std::shared_ptr<Abc> AbcSPtr;
//
// This file uses longer expression std::shared_ptr<> for the purpose of training.
//

class TestFixture
{
public:
    TestFixture() { BOOST_REQUIRE(Setup()); }
    ~TestFixture() { BOOST_REQUIRE(Cleanup()); }

    bool Setup()
    {
        PrintCurrentThreadId(L"TestFixture::Setup");
        return true;
    }

    bool Cleanup()
    {
        PrintCurrentThreadId(L"TestFixture::Cleanup");
        return true;
    }

protected:
    void I_Pass_SPtr_To_You(std::shared_ptr<Abc> const& param)
    {
        // copy of shared pointer reference
        // Note: 'param' is a reference. copy happesn at the next line, not in passing parameter.
        sptr_ = param;
    }

    void Keep_This_Shared_Ptr(std::shared_ptr<TestRootedObject> const& sptr)
    {
        testRootedObjectSPtr_ = sptr;
        VERIFY_IS_TRUE(testRootedObjectSPtr_ != nullptr);

        PrintCurrentThreadId(L"Hey I copied TestRootedObject shared_ptr");

        Threadpool::Post(
            [this]() {
                Sleep(3000);

                VERIFY_IS_TRUE(testRootedObjectSPtr_ != nullptr);

                /////////////////////
                // THE NEXT LINE THROWS!!! 
                // 
                testRootedObjectSPtr_->OnMessage(L"Using copied shared_ptr");
            }
        );
    }

    std::shared_ptr<Abc> sptr_;
    std::shared_ptr<TestRootedObject> testRootedObjectSPtr_;
};

BOOST_FIXTURE_TEST_SUITE(LifeSuite, TestFixture)

BOOST_AUTO_TEST_CASE(SharedPointer)
{
    PrintCurrentThreadId(L"Begin: SharedPointer");

    {
        std::shared_ptr<Abc> sptr;
        {
            auto p = new Abc(L"1st");
            sptr = std::shared_ptr<Abc>(p);
        }
        PrintCurrentThreadId(L"Inside scope: 1st");
    }
    PrintCurrentThreadId(L"Outside scope");

    std::shared_ptr<Abc> fnscope_sptr;

    {
        std::shared_ptr<Abc> sptr = make_shared<Abc>(L"2nd");
        
        // copy sptr
        // equivalent to calling a function void foo(std::shared_ptr<Abc> sptr_parameter)
        // sptr_parameter is a "copy" of sptr
        fnscope_sptr = sptr;

        PrintCurrentThreadId(L"Inside scope: 2nd");
    }

    {
        std::shared_ptr<Abc> sptr = make_shared<Abc>(L"3rd");

        I_Pass_SPtr_To_You(sptr);

        PrintCurrentThreadId(L"Inside scope: 3rd");
    }

    PrintCurrentThreadId(L"End: SharedPointer");
}

BOOST_AUTO_TEST_CASE(WeakPointer)
{
    PrintCurrentThreadId(L"Begin: WeakPointer");

    std::weak_ptr<Abc> abcWPtr;
    {
        std::shared_ptr<Abc> sptr_copy;
        {
            std::shared_ptr<Abc> sptr = make_shared<Abc>(L"original");

            // assign weak_ptr with shared_ptr
            abcWPtr = sptr;

            {
                // lock the shared_ptr
                sptr_copy = abcWPtr.lock();
                VERIFY_IS_TRUE(sptr_copy != nullptr);

                PrintCurrentThreadId(L"weak_ptr lock, scope out");
            }

            PrintCurrentThreadId(L"The original shared_ptr scope out");
        }

        PrintCurrentThreadId(L"The copy shared_ptr scope out");
    }
    PrintCurrentThreadId(L"shared_ptr is gone");

    {
        std::shared_ptr<Abc> sptr_notavailable;

        sptr_notavailable = abcWPtr.lock();
        VERIFY_IS_TRUE(sptr_notavailable == nullptr);   // the weak_ptr can't lock a shared_ptr
    }

    PrintCurrentThreadId(L"End: WeakPointer");
}

BOOST_AUTO_TEST_CASE(NeverDieAsyncOp)
{
    PrintCurrentThreadId(L"NeverDieAsyncOp");

    //
    // positive case
    //
    {
        std::weak_ptr<AsyncOperation> asyncOpWPtr;

        {
            ManualResetEvent mre;
            AsyncOperationSPtr asyncOp = AsyncOperation::CreateAndStart<TestCommon::TestAsyncOperation>(
                [&mre](AsyncOperationSPtr const&) { mre.Set(); },
                AsyncOperationSPtr());

            asyncOpWPtr = asyncOp;

            // wrap async in sync: potential deadlock: avoid this pattern in production
            mre.Wait();

            TestCommon::TestAsyncOperation::End(asyncOp);
        }

        AsyncOperationSPtr lockedAsyncOp = asyncOpWPtr.lock();

        // CONFIRM: the weak pointer can't lock the shared_ptr. The object is released.
        VERIFY_IS_TRUE(lockedAsyncOp == nullptr);
    }

    //
    // negative case: NeverDieAsyncOperation shared_ptr is not released.
    // ==> NeverDieAsyncOperation has a bug!
    //
    {
        std::weak_ptr<AsyncOperation> asyncOpWPtr;

        {
            ManualResetEvent mre;
            AsyncOperationSPtr asyncOp = AsyncOperation::CreateAndStart<NeverDieAsyncOperation>(
                [&mre](AsyncOperationSPtr const&) { mre.Set(); },
                AsyncOperationSPtr());

            asyncOpWPtr = asyncOp;

            // wrap async in sync: potential deadlock: avoid this pattern in production
            mre.Wait();

            TestCommon::TestAsyncOperation::End(asyncOp);
        }

        // CONFIRM: the weak pointer still can lock the shared_ptr.
        //   The object has not been destructed here!!!
        AsyncOperationSPtr lockedAsyncOp = asyncOpWPtr.lock();
        VERIFY_IS_FALSE(lockedAsyncOp == nullptr);
    }
}

BOOST_AUTO_TEST_CASE(FabricComponent)
{
    PrintCurrentThreadId(L"FabricComponent");

    auto fabricComponentSPtr = make_shared<TestFabricComponent>();

    ErrorCode error;

    error = fabricComponentSPtr->Open();
    VERIFY_IS_TRUE(error.IsSuccess());

    // IsOpened() TRUE
    VERIFY_IS_TRUE(fabricComponentSPtr->IsOpened());

    error = fabricComponentSPtr->Close();
    VERIFY_IS_TRUE(error.IsSuccess());

    // IsOpened() FALSE
    VERIFY_IS_FALSE(fabricComponentSPtr->IsOpened());

    fabricComponentSPtr->BeginHelloWorld(
        [fabricComponentSPtr](AsyncOperationSPtr const& op) {

            auto error = fabricComponentSPtr->EndHelloWorld(op);

            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ObjectClosed));
        },
        AsyncOperationSPtr());
}

BOOST_AUTO_TEST_CASE(RootedObject_Member)
{
    PrintCurrentThreadId(L"RootedObject_Member");

    {
        auto fabricComponentSPtr = make_shared<TestFabricComponent>();

        fabricComponentSPtr->Open();

        fabricComponentSPtr->member_obj.OnMessage(L"member");

        fabricComponentSPtr->Close();
    }
    PrintCurrentThreadId(L"Outside the scope");

    Sleep(5000);
}

BOOST_AUTO_TEST_CASE(RootedObject_Unique_Ptr)
{
    PrintCurrentThreadId(L"RootedObject_Unique_Ptr");

    {
        auto fabricComponentSPtr = make_shared<TestFabricComponent>();

        fabricComponentSPtr->Open();

        fabricComponentSPtr->member_obj.OnMessage(L"Unique_Ptr");

        fabricComponentSPtr->Close();
    }
    PrintCurrentThreadId(L"Outside the scope");

    Sleep(5000);
}

BOOST_AUTO_TEST_CASE(RootedObject_Shared_Ptr)
{
    PrintCurrentThreadId(L"RootedObject_Shared_Ptr");

    {
        auto fabricComponentSPtr = make_shared<TestFabricComponent>();

        fabricComponentSPtr->Open();

        Keep_This_Shared_Ptr(fabricComponentSPtr->shared_ptr_obj);

        fabricComponentSPtr->Close();
    }
    PrintCurrentThreadId(L"Outside the scope");

    Sleep(10000);
}

BOOST_AUTO_TEST_SUITE_END()

}