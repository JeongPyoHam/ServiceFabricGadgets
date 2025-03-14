#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Common.h"

#include "TestTools.h"

using namespace std;
using namespace Common;

namespace AsyncOperationPlay
{

class BasicAsyncOperation : public AsyncOperation
{
public:
    BasicAsyncOperation(AsyncCallback const & callback, AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
    { }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        PrintCurrentThreadId(L"Basic::OnStart");

        TryComplete(thisSPtr, ErrorCodeValue::NotImplemented);
    }
};

class TestFixture
{
protected:
};

BOOST_FIXTURE_TEST_SUITE(Source1Suite, TestFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
    PrintCurrentThreadId(L"Basic");

    auto callback = [](AsyncOperationSPtr const& operation) {
        ErrorCode error = AsyncOperation::End(operation);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotImplemented));
    };

    auto basicOp = AsyncOperation::CreateAndStart<BasicAsyncOperation>(callback, AsyncOperationSPtr());
}

BOOST_AUTO_TEST_SUITE_END()

}