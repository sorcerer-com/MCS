using CodeGenerator.Infos;
using CodeGenerator.Utils;
using System.Collections.Generic;

namespace CodeGenerator.Generators.Wrappers
{
    public class FunctionGenerator : BaseGenerator
    {
        public enum FunctionGeneratorType
        {
            Header_Only,
            Header,
            Code
        }

        public FunctionGeneratorType Type { get; set; }


        public FunctionGenerator(CodeGenerator owner, FunctionGeneratorType type)
            : base(owner)
        {
            this.Type = type;
        }


        public override List<string> Generate(string _class)
        {
            List<string> result = new List<string>();

            List<Function> functions = this.CodeGenerator.Functions;
            foreach (var func in functions)
            {
                if (func.Access == "public" && func.Class == _class &&
                    func.ContainsAttribute(FunctionAttributes.Wrap))
                {
                    string wrapperReturnType = WrapperTypesUtils.GetWrapperType(func.ReturnType);

                    List<string> wrapperParamsTypes = new List<string>();
                    foreach (var pair in func.Parameters)
                        wrapperParamsTypes.Add(WrapperTypesUtils.GetWrapperType(FunctionGenerator.GetClearParameterType(pair.Value)));
                    
                    string args = string.Empty;
                    int i = 0;
                    foreach (var pair in func.Parameters)
                    {
                        string param = pair.Key;
                        if (param.Contains("="))
                            param = param.Remove(param.IndexOf("=")).Trim();
                        args += string.Format("{0} {1}, ", wrapperParamsTypes[i++], param);
                    }
                    if (args.Length > 2)
                        args = args.Remove(args.Length - 2);

                    if (this.Type == FunctionGeneratorType.Header_Only)
                        result.Add("{0} {1}({2})", wrapperReturnType, func.Name, args);
                    else if (this.Type == FunctionGeneratorType.Header)
                        result.Add("{0} {1}({2});", wrapperReturnType, func.Name, args);
                    else // Code
                        result.Add("{0} M{1}::{2}({3})", wrapperReturnType, func.Class, func.Name, args);
                    if (this.Type != FunctionGeneratorType.Header)
                    {
                        args = string.Empty;
                        i = 0;
                        foreach (var pair in func.Parameters)
                        {
                            string param = pair.Key;
                            if (param.Contains("="))
                                param = param.Remove(param.IndexOf("=")).Trim();
                            args += WrapperTypesUtils.ConvertFromWrapperType(param, FunctionGenerator.GetClearParameterType(pair.Value), wrapperParamsTypes[i++]) + ", ";
                        }
                        if (args.Length > 2)
                            args = args.Remove(args.Length - 2);

                        string call = string.Format("{0}->{1}({2})", FunctionGenerator.GetAccessMember(func), func.Name, args);

                        result.Add("{");
                        if (wrapperReturnType == "void")
                        {
                            result.Add("{0}{1};", CodeGenerator.Indent, call);
                            if (!func.Constant && !func.ContainsAttribute(FunctionAttributes.Const)) // function should call onChanged
                                result.Add("{0}{1}", CodeGenerator.Indent, FunctionGenerator.GetOnChangedCall(func, wrapperReturnType));
                        }
                        else if (wrapperReturnType.StartsWith("List<"))
                        {
                            result.AddRange(WrapperTypesUtils.ConvertToWrapperListType(call, func.ReturnType, wrapperReturnType, func.Name, 1));
                            if (!func.Constant && !func.ContainsAttribute(FunctionAttributes.Const)) // function should call onChanged
                                result.Add("{0}{1}", CodeGenerator.Indent, FunctionGenerator.GetOnChangedCall(func, wrapperReturnType));
                            result.Add("{0}return collection;", CodeGenerator.Indent);
                        }
                        else
                        {
                            string converted = WrapperTypesUtils.ConvertToWrapperType(call, func.ReturnType, wrapperReturnType, null);
                            if (!func.Constant && !func.ContainsAttribute(FunctionAttributes.Const)) // function should call onChanged
                            {
                                if (wrapperReturnType == "bool")
                                {
                                    result.Add("{0}bool res = {1};", CodeGenerator.Indent, converted);
                                    result.Add("{0}if (res)", CodeGenerator.Indent);
                                    result.Add("{0}{0}{1}", CodeGenerator.Indent, FunctionGenerator.GetOnChangedCall(func, wrapperReturnType));
                                    result.Add("{0}return res;", CodeGenerator.Indent);
                                }
                                else if (wrapperReturnType == "MSceneElement^" || wrapperReturnType == "MContentElement^")
                                {
                                    result.Add("{0}{1} res = {2};", CodeGenerator.Indent, wrapperReturnType, converted);
                                    result.Add("{0}if (res != nullptr)", CodeGenerator.Indent);
                                    result.Add("{0}{0}{1}", CodeGenerator.Indent, FunctionGenerator.GetOnChangedCall(func, wrapperReturnType));
                                    result.Add("{0}return res;", CodeGenerator.Indent);
                                }
                            }
                            else
                                result.Add("{0}return {1};", CodeGenerator.Indent, converted);
                        }
                        result.Add("}");
                        result.Add("");
                    }
                    if (func.ContainsAttribute(FunctionAttributes.EndGroup))
                        result.Add("");
                }
            }
            if (result.Count > 0 && string.IsNullOrWhiteSpace(result[result.Count - 1]))
                result.RemoveAt(result.Count - 1); // remove last empty line
            
            return result;
        }


        public static string GetAccessMember(Function func)
        {
            return "this->" + char.ToLower(func.Class[0]) + func.Class.Substring(1);
        }

        public static string GetClearParameterType(string type)
        {
            if (type.StartsWith("const "))
                type = type.Remove(0, "const ".Length);
            if (type.EndsWith("&") || type.EndsWith("*"))
                type = type.Remove(type.Length - 1);
            return type;
        }

        public static string GetOnChangedCall(Function func, string wrapperReturnType)
        {
            if (func.Class.EndsWith("Manager") && (wrapperReturnType == "MSceneElement^" || wrapperReturnType == "MContentElement^"))
                return "this->OnChanged(res);";
            else if (func.Class.EndsWith("Manager"))
                return "this->OnChanged(nullptr);";
            return "this->OnChanged();";
        }

    }
}
