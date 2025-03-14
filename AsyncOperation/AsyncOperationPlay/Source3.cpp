#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Common.h"

#include "TestTools.h"

using namespace std;
using namespace Common;

namespace AsyncOperationPlay
{

class LoopedAsyncOperation;

class TestFixture
{
public:
    int count;

    void Callback(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously) {

        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = AsyncOperation::End(operation);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotImplemented));

        if (count < 5)
        {
            auto basicOp = AsyncOperation::CreateAndStart<LoopedAsyncOperation>(
                *this,
                count,
                std::bind(&TestFixture::Callback, this, placeholders::_1, false),
                AsyncOperationSPtr());
            Callback(basicOp, true);
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

BOOST_FIXTURE_TEST_SUITE(Source3Suite, TestFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
    count = 0;

    PrintCurrentThreadId(L"Basic");

    auto basicOp = AsyncOperation::CreateAndStart<LoopedAsyncOperation>(
        *this,
        count,
        std::bind(&TestFixture::Callback, this, placeholders::_1,false),
        AsyncOperationSPtr());
    Callback(basicOp, true);
}

BOOST_AUTO_TEST_SUITE_END()

}