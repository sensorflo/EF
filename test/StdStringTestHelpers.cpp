#include "StdStringTestHelpers.h"

#include <gmock/gmock.h>

#include <boost/tokenizer.hpp>

#include <codecvt>

//////////////////////////////////////////////////////////////////////////
namespace
{
	std::string ToString(const std::wstring& str)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    	return converter.to_bytes(str.data(), str.data() + str.size());
	}

	std::vector<std::string> ToLines(const std::wstring& wstr)
	{
		const auto& str = ToString(wstr);
		const boost::tokenizer tokenizer{str, boost::char_separator<char>{"\n", "", boost::keep_empty_tokens}};
		return {tokenizer.begin(), tokenizer.end()};
	}

	//////////////////////////////////////////////////////////////////////////
	class LiteralStrEqMatcher
	{
	public:
		using is_gtest_matcher = void;

		explicit LiteralStrEqMatcher(std::wstring expected)
		: m_Expected{std::move(expected)}
		{
		}

		bool MatchAndExplain(const std::wstring& actual, std::ostream* pOStream) const
		{
			const auto eitherHasNewlines = m_Expected.find_first_of(L"\n\r\f") != std::wstring::npos || actual.find_first_of(L"\n\r\f") != std::wstring::npos;
			if (pOStream != nullptr && eitherHasNewlines)
			{
				*pOStream << "\n  Expected literally:\n" << ToString(m_Expected);
				*pOStream << "\n  Actual literally:\n" << ToString(actual);
				const auto diff = testing::internal::edit_distance::CreateUnifiedDiff(ToLines(m_Expected), ToLines(actual));
				*pOStream << "\n  With diff: \n" << diff; // Same output as in EXPECT_EQ(myString1, myString2)
			}
			return actual == m_Expected;
		}

		void DescribeTo(std::ostream* pOstream) const
		{
			*pOstream << "is equal";
			PrintExpected(*pOstream);
		}

		void DescribeNegationTo(std::ostream* pOstream) const
		{
			*pOstream << "is not equal";
			PrintExpected(*pOstream);
		}

	private:
		void PrintExpected(std::ostream& ostream) const
		{
			// Its only "Expect:" so the value lines up with the following "Actual:" line from Google test.
			ostream << "\n  Expect: " << testing::PrintToString(m_Expected);
		}

		std::wstring m_Expected;
	};
}

//////////////////////////////////////////////////////////////////////////
::testing::Matcher<const std::wstring&> LiteralStrEq(std::wstring expected)
{
	return LiteralStrEqMatcher{std::move(expected)};
}