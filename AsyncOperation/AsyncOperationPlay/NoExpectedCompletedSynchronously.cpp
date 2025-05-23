#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Common.h"

#include "TestTools.h"

using namespace std;
using namespace Common;

namespace NoExpectedCompletedSynchronously
{

class LoopedAsyncOperation;

//
// An example that doesn't use expectedCompletedSynchronously pattern. Set a break point and observe how deep the stack is!!!
//
class CallManySyncAsyncOperation
    : public AsyncOperation
{
public:
    CallManySyncAsyncOperation(AsyncCallback const & callback, AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
    { }

    virtual ~CallManySyncAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        return AsyncOperation::End(operation);
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const& thisSPtr) override
    {
        // call 1st async op
        auto firstOp = AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            [this](AsyncOperationSPtr const & first) {
                OnFirstCompleted(first);
            },
            thisSPtr);
    }

    void OnFirstCompleted(AsyncOperationSPtr const& firstOp)
    {
        auto error = CompletedAsyncOperation::End(firstOp);
        if (!error.IsSuccess())
        {
            // child operation's Parent is thisSPtr
            TryComplete(firstOp->Parent, error);
            return;
        }

        auto thisSPtr = firstOp->Parent;

        // call 2nd async op
        auto secondOp = AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            [this](AsyncOperationSPtr const& second) {
                OnSecondCompleted(second);
            },
            thisSPtr);
    }

    void OnSecondCompleted(AsyncOperationSPtr const& secondOp)
    {
        auto error = CompletedAsyncOperation::End(secondOp);
        if (!error.IsSuccess())
        {
            // child operation's Parent is thisSPtr
            TryComplete(secondOp->Parent, error);
            return;
        }

        auto thisSPtr = secondOp->Parent;

        // call 3rd async op
        auto thirdOp = AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            [this](AsyncOperationSPtr const& third) {
                OnThirdCompleted(third);
            },
            thisSPtr);
    }

    void OnThirdCompleted(AsyncOperationSPtr const& thirdOp)
    {
        auto error = CompletedAsyncOperation::End(thirdOp);

        // child operation's Parent is thisSPtr
        TryComplete(thirdOp->Parent, error);
    }
};

class TestFixture
{
public:
    int count;

    void Callback(AsyncOperationSPtr const& operation) {
        ErrorCode error = AsyncOperation::End(operation);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotImplemented));

        if (count < 5)
        {
            auto basicOp = AsyncOperation::CreateAndStart<LoopedAsyncOperation>(
                *this,
                count,
                std::bind(&TestFixture::Callback, this, placeholders::_1),
                AsyncOperationSPtr());
        }
    };
};

class LoopedAsyncOperation : public AsyncOperation
{
public:
    LoopedAsyncOperation(TestFixture& fixture, int i, AsyncCallback const& callback, AsyncOperationSPtr const& parent)
        : AsyncOperation(callback, parent)
        , fixture_(fixture)
        , i_(i)
    {
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const& thisSPtr) override
    {
        fixture_.count++;

        PrintCurrentThreadId(L"Basic::OnStart");

        TryComplete(thisSPtr, ErrorCodeValue::NotImplemented);
    }

    TestFixture& fixture_;
    int i_;
};

BOOST_FIXTURE_TEST_SUITE(Source2Suite, TestFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
    count = 0;

    PrintCurrentThreadId(L"Basic");

    auto basicOp = AsyncOperation::CreateAndStart<LoopedAsyncOperation>(
        *this,
        count,
        std::bind(&TestFixture::Callback, this, placeholders::_1),
        AsyncOperationSPtr());
}

BOOST_AUTO_TEST_CASE(Unfolded)
{
    count = 0;

    PrintCurrentThreadId(L"Unfolded");

    ManualResetEvent mre;

    // Invoke a AsyncOperation
    auto operation = AsyncOperation::CreateAndStart<CallManySyncAsyncOperation>(
        [&mre](AsyncOperationSPtr const&) {
            mre.Set();
        },
        AsyncOperationSPtr());

    // async-wrap-to-sync
    // Avoid this as much as possible in production code
    // Here for testing
    mre.Wait();

    // this error returns to the caller typically.
    ErrorCode error = CallManySyncAsyncOperation::End(operation);


    VERIFY_IS_TRUE(error.IsSuccess());
}


BOOST_AUTO_TEST_SUITE_END()

}