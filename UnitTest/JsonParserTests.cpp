#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/JsonParser.h"
#include "GitHubReleaseSnapshot.h"




using namespace Microsoft::VisualStudio::CppUnitTestFramework;




namespace UnitTest
{
    TEST_CLASS(JsonParserTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }


        // ------------------------------------------------------------------ //
        // Strict parsing + typed accessors
        // ------------------------------------------------------------------ //

        TEST_METHOD(Parse_SimpleObject_ReadsTypedValues)
        {
            string         strJson = R"({ "name": "Casso", "count": 42, "on": true })";
            JsonValue      root;
            JsonParseError err;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::IsTrue (root.GetType() == JsonType::Object);

            string strName;
            int    iCount = 0;
            bool   fOn    = false;

            Assert::AreEqual (S_OK, root.GetString ("name", strName));
            Assert::AreEqual (string ("Casso"), strName);

            Assert::AreEqual (S_OK, root.GetInt ("count", iCount));
            Assert::AreEqual (42, iCount);

            Assert::AreEqual (S_OK, root.GetBool ("on", fOn));
            Assert::IsTrue (fOn);
        }


        TEST_METHOD(Parse_MissingKey_ReturnsKeyMissing)
        {
            string         strJson = R"({ "a": 1 })";
            JsonValue      root;
            JsonParseError err;
            string         strOut;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::AreEqual (JSON_E_KEY_MISSING, root.GetString ("missing", strOut));
        }


        TEST_METHOD(Parse_TypeMismatch_ReturnsTypeMismatch)
        {
            string         strJson = R"({ "a": 1 })";
            JsonValue      root;
            JsonParseError err;
            string         strOut;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::AreEqual (JSON_E_TYPE_MISMATCH, root.GetString ("a", strOut));
        }


        TEST_METHOD(Parse_NestedArrayAndObject_Walks)
        {
            string         strJson = R"({ "list": [ { "k": "v" } ] })";
            JsonValue      root;
            JsonParseError err;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));

            const JsonValue * list = nullptr;
            Assert::AreEqual (S_OK, root.GetArray ("list", list));
            Assert::AreEqual (size_t (1), list->ArraySize());

            string strK;
            Assert::AreEqual (S_OK, list->ArrayAt (0).GetString ("k", strK));
            Assert::AreEqual (string ("v"), strK);
        }


        TEST_METHOD(Parse_StrictRejectsComments)
        {
            string         strJson = "{ \"a\": 1 // trailing\n }";
            JsonValue      root;
            JsonParseError err;

            Assert::IsTrue (FAILED (JsonParser::Parse (strJson, root, err)));
        }


        TEST_METHOD(Parse_StrictRejectsTrailingComma)
        {
            string         strJson = R"({ "a": 1, })";
            JsonValue      root;
            JsonParseError err;

            Assert::IsTrue (FAILED (JsonParser::Parse (strJson, root, err)));
        }


        // ------------------------------------------------------------------ //
        // JSONC tolerance
        // ------------------------------------------------------------------ //

        TEST_METHOD(ParseJsonc_AllowsLineComments)
        {
            string         strJson = "{\n  // a comment\n  \"a\": 1\n}";
            JsonValue      root;
            JsonParseError err;
            int            iA      = 0;

            Assert::AreEqual (S_OK, JsonParser::ParseJsonc (strJson, root, err));
            Assert::AreEqual (S_OK, root.GetInt ("a", iA));
            Assert::AreEqual (1, iA);
        }


        TEST_METHOD(ParseJsonc_AllowsBlockComments)
        {
            string         strJson = "{ /* block */ \"a\": /* mid */ 2 }";
            JsonValue      root;
            JsonParseError err;
            int            iA      = 0;

            Assert::AreEqual (S_OK, JsonParser::ParseJsonc (strJson, root, err));
            Assert::AreEqual (S_OK, root.GetInt ("a", iA));
            Assert::AreEqual (2, iA);
        }


        TEST_METHOD(ParseJsonc_AllowsTrailingCommaInObjectAndArray)
        {
            string         strJson = R"({ "list": [ 1, 2, ], "a": 3, })";
            JsonValue      root;
            JsonParseError err;
            int            iA      = 0;

            Assert::AreEqual (S_OK, JsonParser::ParseJsonc (strJson, root, err));
            Assert::AreEqual (S_OK, root.GetInt ("a", iA));
            Assert::AreEqual (3, iA);

            const JsonValue * list = nullptr;
            Assert::AreEqual (S_OK, root.GetArray ("list", list));
            Assert::AreEqual (size_t (2), list->ArraySize());
        }


        // ------------------------------------------------------------------ //
        // Source-span tracking (for locate-and-splice)
        // ------------------------------------------------------------------ //

        TEST_METHOD(Span_StringValue_CoversQuotedToken)
        {
            string         strJson = R"({ "name": "abc" })";
            JsonValue      root;
            JsonParseError err;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));

            const JsonValue & name = root.GetObjectEntries()[0].second;

            // Span includes the surrounding quotes; content is span+1 .. span end-1.
            Assert::AreEqual (string ("\"abc\""), strJson.substr (name.SpanBegin(), name.SpanEnd() - name.SpanBegin()));
            Assert::AreEqual (string ("abc"), strJson.substr (name.SpanBegin() + 1, name.SpanEnd() - name.SpanBegin() - 2));
        }


        TEST_METHOD(Span_Object_CoversBraces)
        {
            string         strJson = R"({ "defaults": { "x": 1 } })";
            JsonValue      root;
            JsonParseError err;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));

            const JsonValue * defaults = nullptr;
            Assert::AreEqual (S_OK, root.GetObject ("defaults", defaults));

            string strSpan = strJson.substr (defaults->SpanBegin(), defaults->SpanEnd() - defaults->SpanBegin());
            Assert::AreEqual (string ("{ \"x\": 1 }"), strSpan);
        }


        TEST_METHOD(Span_SpliceFontFaceValue_PreservesRest)
        {
            //
            // Mimics the Windows Terminal edit: locate profiles.defaults.fontFace
            // and splice ONLY its value, leaving comments and layout intact.
            //
            string strJson =
                "{\n"
                "    // user comment\n"
                "    \"profiles\": {\n"
                "        \"defaults\": { \"fontFace\": \"Consolas\" },\n"
                "        \"list\": [ { \"name\": \"PS\", \"fontFace\": \"Cascadia Code\" } ]\n"
                "    }\n"
                "}";

            JsonValue      root;
            JsonParseError err;

            Assert::AreEqual (S_OK, JsonParser::ParseJsonc (strJson, root, err));

            const JsonValue * profiles = root.FindObject ("profiles");
            Assert::IsNotNull (profiles);

            const JsonValue * defaults = profiles->FindObject ("defaults");
            Assert::IsNotNull (defaults);

            const JsonValue * face = defaults->FindMember ("fontFace");
            Assert::IsNotNull (face);

            // Replace just the inner content (between the quotes).
            size_t contentBegin = face->SpanBegin() + 1;
            size_t contentEnd   = face->SpanEnd() - 1;
            string strEdited    = strJson.substr (0, contentBegin) + "CaskaydiaCove Nerd Font" + strJson.substr (contentEnd);

            Assert::IsTrue (strEdited.find ("\"fontFace\": \"CaskaydiaCove Nerd Font\"") != string::npos);
            Assert::IsTrue (strEdited.find ("// user comment") != string::npos);
            Assert::IsTrue (strEdited.find ("\"fontFace\": \"Cascadia Code\"") != string::npos);
        }


        // ------------------------------------------------------------------ //
        // Real GitHub API payload snapshot (the install-nerd-fonts flow)
        // ------------------------------------------------------------------ //

        TEST_METHOD(Snapshot_GitHubRelease_ParsesAndExtractsTag)
        {
            string         strJson = GetGitHubReleaseSnapshot ();
            JsonValue      root;
            JsonParseError err;
            string         strTag;

            // ~227 KB real ryanoasis/nerd-fonts releases/latest response.
            Assert::AreEqual (size_t (226870), strJson.size());

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::IsTrue (root.GetType() == JsonType::Object);

            Assert::AreEqual (S_OK, root.GetString ("tag_name", strTag));
            Assert::AreEqual (string ("v3.4.0"), strTag);
        }


        TEST_METHOD(Snapshot_GitHubRelease_WalksNestedAssets)
        {
            string         strJson = GetGitHubReleaseSnapshot ();
            JsonValue      root;
            JsonParseError err;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));

            // Nested author object.
            const JsonValue * author = root.FindObject ("author");
            Assert::IsNotNull (author);

            string strLogin;
            Assert::AreEqual (S_OK, author->GetString ("login", strLogin));
            Assert::AreEqual (string ("github-actions[bot]"), strLogin);

            // Assets array with object elements carrying the download URL.
            const JsonValue * assets = nullptr;
            Assert::AreEqual (S_OK, root.GetArray ("assets", assets));
            Assert::IsTrue (assets->ArraySize() > 0);

            string strName;
            Assert::AreEqual (S_OK, assets->ArrayAt (0).GetString ("name", strName));
            Assert::IsFalse (strName.empty());

            string strUrl;
            Assert::AreEqual (S_OK, assets->ArrayAt (0).GetString ("browser_download_url", strUrl));
            Assert::IsTrue (strUrl.find ("v3.4.0") != string::npos);
        }


        TEST_METHOD(Snapshot_GitHubRelease_TruncatedNeverHangsOrCrashes)
        {
            //
            // A partial network read yields a truncated body.  Every prefix must
            // terminate (no infinite loop) and fail gracefully (no crash).
            //
            string strFull = GetGitHubReleaseSnapshot ();

            for (size_t pct = 1; pct < 100; pct += 7)
            {
                string         strCut = strFull.substr (0, strFull.size() * pct / 100);
                JsonValue      root;
                JsonParseError err;

                // Truncated mid-structure: must fail, must return.
                Assert::IsTrue (FAILED (JsonParser::Parse (strCut, root, err)));
            }
        }


        // ------------------------------------------------------------------ //
        // Recursion depth guard (pathological deeply nested input)
        // ------------------------------------------------------------------ //

        TEST_METHOD(Depth_DeeplyNestedArrays_FailsGracefully)
        {
            string         strJson (5000, '[');
            JsonValue      root;
            JsonParseError err;

            // Must NOT overflow the stack — graceful failure instead.
            Assert::IsTrue (FAILED (JsonParser::Parse (strJson, root, err)));
            Assert::AreEqual (string ("Maximum nesting depth exceeded"), err.message);
        }


        TEST_METHOD(Depth_DeeplyNestedObjects_FailsGracefully)
        {
            string strJson;

            for (int i = 0; i < 5000; i++)
            {
                strJson += "{\"a\":";
            }

            JsonValue      root;
            JsonParseError err;

            Assert::IsTrue (FAILED (JsonParser::Parse (strJson, root, err)));
            Assert::AreEqual (string ("Maximum nesting depth exceeded"), err.message);
        }


        TEST_METHOD(Depth_WithinLimit_Succeeds)
        {
            //
            // 150 levels of valid nested arrays parse fine (under the 200 cap).
            //
            string         strJson = string (150, '[') + string (150, ']');
            JsonValue      root;
            JsonParseError err;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::IsTrue (root.GetType() == JsonType::Array);
        }


        // ------------------------------------------------------------------ //
        // Scalars, escapes, numbers, structure edge cases
        // ------------------------------------------------------------------ //

        TEST_METHOD(Parse_EmptyObjectAndArray)
        {
            JsonValue      root;
            JsonParseError err;
            string         strObj  = "{}";
            string         strArr  = "[]";

            Assert::AreEqual (S_OK, JsonParser::Parse (strObj, root, err));
            Assert::IsTrue (root.GetType() == JsonType::Object);

            JsonValue      arr;
            JsonParseError err2;

            Assert::AreEqual (S_OK, JsonParser::Parse (strArr, arr, err2));
            Assert::IsTrue (arr.GetType() == JsonType::Array);
            Assert::AreEqual (size_t (0), arr.ArraySize());
        }


        TEST_METHOD(Parse_NumberForms)
        {
            string         strJson = R"({ "neg": -17, "frac": 3.5, "exp": 6.02e2, "zero": 0, "negexp": -1.5e-1 })";
            JsonValue      root;
            JsonParseError err;
            double         d       = 0.0;
            int            i       = 0;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));

            Assert::AreEqual (S_OK, root.GetInt ("neg", i));
            Assert::AreEqual (-17, i);

            Assert::AreEqual (S_OK, root.GetNumber ("frac", d));
            Assert::AreEqual (3.5, d, 1e-9);

            Assert::AreEqual (S_OK, root.GetNumber ("exp", d));
            Assert::AreEqual (602.0, d, 1e-6);

            Assert::AreEqual (S_OK, root.GetNumber ("negexp", d));
            Assert::AreEqual (-0.15, d, 1e-9);
        }


        TEST_METHOD(Parse_StringEscapes)
        {
            string         strJson = R"({ "s": "a\tb\nc\r\"d\\e\/f" })";
            JsonValue      root;
            JsonParseError err;
            string         strS;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::AreEqual (S_OK, root.GetString ("s", strS));
            Assert::AreEqual (string ("a\tb\nc\r\"d\\e/f"), strS);
        }


        TEST_METHOD(Parse_UnicodeEscape_AsciiRange)
        {
            string         strJson = R"({ "s": "A\u0042C" })";
            JsonValue      root;
            JsonParseError err;
            string         strS;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::AreEqual (S_OK, root.GetString ("s", strS));
            Assert::AreEqual (string ("ABC"), strS);
        }


        TEST_METHOD(Parse_NullAndBooleans)
        {
            string         strJson = R"({ "n": null, "t": true, "f": false })";
            JsonValue      root;
            JsonParseError err;
            bool           b       = true;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));

            const JsonValue * n = root.FindMember ("n");
            Assert::IsNotNull (n);
            Assert::IsTrue (n->GetType() == JsonType::Null);

            Assert::AreEqual (S_OK, root.GetBool ("f", b));
            Assert::IsFalse (b);
        }


        TEST_METHOD(Parse_WhitespaceAroundTokens)
        {
            string         strJson = "  \n\t { \"a\" : \n 1 , \"b\" : [ 2 , 3 ] }  \n ";
            JsonValue      root;
            JsonParseError err;
            int            iA      = 0;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::AreEqual (S_OK, root.GetInt ("a", iA));
            Assert::AreEqual (1, iA);
        }


        TEST_METHOD(Parse_UnterminatedString_Fails)
        {
            JsonValue      root;
            JsonParseError err;
            string         strJson = R"({ "a": "no end )";

            Assert::IsTrue (FAILED (JsonParser::Parse (strJson, root, err)));
        }


        TEST_METHOD(Parse_InvalidEscape_Fails)
        {
            JsonValue      root;
            JsonParseError err;
            string         strJson = R"({ "a": "bad\xescape" })";

            Assert::IsTrue (FAILED (JsonParser::Parse (strJson, root, err)));
        }


        TEST_METHOD(Parse_TrailingContent_Fails)
        {
            JsonValue      root;
            JsonParseError err;
            string         strJson = R"({ "a": 1 } garbage)";

            Assert::IsTrue (FAILED (JsonParser::Parse (strJson, root, err)));
        }


        TEST_METHOD(Parse_NonStringKey_Fails)
        {
            JsonValue      root;
            JsonParseError err;
            string         strJson = R"({ 1: 2 })";

            Assert::IsTrue (FAILED (JsonParser::Parse (strJson, root, err)));
        }


        TEST_METHOD(Parse_EmptyInput_Fails)
        {
            JsonValue      root;
            JsonParseError err;
            string         strEmpty = "";
            string         strBlank = "    ";

            Assert::IsTrue (FAILED (JsonParser::Parse (strEmpty, root, err)));
            Assert::IsTrue (FAILED (JsonParser::Parse (strBlank, root, err)));
        }


        TEST_METHOD(Parse_TopLevelScalars)
        {
            JsonValue      root;
            JsonParseError err;
            string         strNum  = "42";

            Assert::AreEqual (S_OK, JsonParser::Parse (strNum, root, err));
            Assert::IsTrue (root.GetType() == JsonType::Number);
            Assert::AreEqual (42, root.GetInt());

            JsonValue      str;
            JsonParseError err2;
            string         strStr  = R"("hello")";

            Assert::AreEqual (S_OK, JsonParser::Parse (strStr, str, err2));
            Assert::IsTrue (str.GetType() == JsonType::String);
            Assert::IsTrue (str.GetString() == "hello");
        }


        TEST_METHOD(Parse_LargeFlatArray)
        {
            string strJson = "[";

            for (int i = 0; i < 1000; i++)
            {
                if (i > 0)
                {
                    strJson += ",";
                }
                strJson += std::to_string (i);
            }
            strJson += "]";

            JsonValue      root;
            JsonParseError err;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::AreEqual (size_t (1000), root.ArraySize());
            Assert::AreEqual (500, root.ArrayAt (500).GetInt());
        }


        // ------------------------------------------------------------------ //
        // Zero-copy redesign: string_view values/keys + owning JsonDocument
        // ------------------------------------------------------------------ //

        TEST_METHOD(Parse_EmptyStringValue)
        {
            string         strJson = R"({ "k": "" })";
            JsonValue      root;
            JsonParseError err;
            string         strS    = "sentinel";

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::AreEqual (S_OK, root.GetString ("k", strS));
            Assert::IsTrue (strS.empty());
        }


        TEST_METHOD(Parse_EscapeAtStartOfString)
        {
            // The very first content byte is an escape, so the decoded buffer is
            // seeded from a zero-length raw run before decoding continues.
            string         strJson = R"({ "k": "\tabc" })";
            JsonValue      root;
            JsonParseError err;
            string         strS;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::AreEqual (S_OK, root.GetString ("k", strS));
            Assert::AreEqual (string ("\tabc"), strS);
        }


        TEST_METHOD(Parse_EscapedKey_LookupUsesDecodedKey)
        {
            // Keys are zero-copy too, decoded into an owned buffer when escaped.
            // Lookup must match the DECODED key, not the raw bytes.
            string         strJson = R"({ "a\tb": 42 })";
            JsonValue      root;
            JsonParseError err;
            int            v       = 0;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::AreEqual (S_OK, root.GetInt ("a\tb", v));
            Assert::AreEqual (42, v);
        }


        TEST_METHOD(Parse_HexNumber)
        {
            string         strJson = R"({ "n": 0x1F })";
            JsonValue      root;
            JsonParseError err;
            int            n       = 0;

            Assert::AreEqual (S_OK, JsonParser::Parse (strJson, root, err));
            Assert::AreEqual (S_OK, root.GetInt ("n", n));
            Assert::AreEqual (31, n);
        }


        TEST_METHOD(ZeroCopy_EscapeFreeValue_ViewsIntoSource)
        {
            JsonParseError err;
            JsonDocument   doc;

            Assert::AreEqual (S_OK, doc.Parse (string (R"({ "k": "plain" })"), err));

            const string &  src = doc.Source();
            string_view     v   = doc.Root().GetObjectEntries()[0].second.GetString();

            Assert::IsTrue (v == "plain");

            // An escape-free value is a view INTO the document's source buffer.
            Assert::IsTrue (v.data() >= src.data() && v.data() < src.data() + src.size());
        }


        TEST_METHOD(ZeroCopy_EscapedValue_OwnedOutsideSource)
        {
            JsonParseError err;
            JsonDocument   doc;

            Assert::AreEqual (S_OK, doc.Parse (string (R"({ "k": "a\tb" })"), err));

            const string &  src = doc.Source();
            string_view     v   = doc.Root().GetObjectEntries()[0].second.GetString();

            Assert::IsTrue (v == "a\tb");

            // An escaped value is decoded into an owned buffer, NOT a source view.
            bool fInSource = (v.data() >= src.data() && v.data() < src.data() + src.size());
            Assert::IsFalse (fInSource);
        }


        TEST_METHOD(JsonDocument_OwnsSource_ParsedFromTemporary)
        {
            JsonParseError err;
            JsonDocument   doc;

            // Parse from a temporary: the document must take ownership of the
            // bytes so the zero-copy views remain valid after the call.
            Assert::AreEqual (S_OK, doc.Parse (string (R"({ "k": "hello", "n": 7 })"), err));

            string strK;
            Assert::AreEqual (S_OK, doc.Root().GetString ("k", strK));
            Assert::AreEqual (string ("hello"), strK);

            int n = 0;
            Assert::AreEqual (S_OK, doc.Root().GetInt ("n", n));
            Assert::AreEqual (7, n);

            Assert::IsTrue (doc.Source().find ("hello") != string::npos);
        }


        TEST_METHOD(JsonDocument_Jsonc_AllowsComments)
        {
            JsonParseError err;
            JsonDocument   doc;

            Assert::AreEqual (S_OK, doc.ParseJsonc (string ("{ // c\n \"k\": 1, }"), err));

            int v = 0;
            Assert::AreEqual (S_OK, doc.Root().GetInt ("k", v));
            Assert::AreEqual (1, v);
        }


        TEST_METHOD(JsonDocument_MoveKeepsViewsValid)
        {
            JsonParseError err;
            JsonDocument   doc;

            Assert::AreEqual (S_OK, doc.Parse (string (R"({ "k": "value-here" })"), err));

            string_view  before  = doc.Root().GetObjectEntries()[0].second.GetString();
            const char * pBefore = before.data();

            JsonDocument doc2 = move (doc);

            // The source is held behind a unique_ptr, so moving the document does
            // not relocate the buffer: the pre-move view still reads the same bytes.
            Assert::IsTrue (before == "value-here");

            string_view after = doc2.Root().GetObjectEntries()[0].second.GetString();
            Assert::IsTrue (after == "value-here");
            Assert::IsTrue (after.data() == pBefore);
        }
    };
}
