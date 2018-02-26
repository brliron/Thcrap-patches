using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace Squirrel_trace_viewer
{
    public abstract class AElement
    {
        public abstract string Description();

        public override string ToString() { return Description(); }

        public virtual string GetTreeLabel() => this.Description();

        public virtual void AddChildsToTree(ItemCollection items, List<AElement> stack)
        { }

        public static AElement Create(JToken obj, Dictionary<UInt32, JToken> objects)
        {
            switch (obj.Type)
            {
                case JTokenType.Null:
                    return new Null();

                case JTokenType.Boolean:
                    return new Boolean(obj);

                case JTokenType.Integer:
                    return new Integer(obj);

                case JTokenType.Float:
                    return new Float(obj);

                case JTokenType.String:
                    if (AObject.IsAddr((string)obj))
                        return new Reference(obj, objects);
                    else
                        return new String(obj);

                case JTokenType.Object:
                    return new JsonObject(obj, objects);

                case JTokenType.Array:
                    return new JsonArray(obj, objects);

                default:
                    return new String(obj.ToString());
            }
        }
    }

    class Instruction : AElement
    {
        public string op { get; }
        public AElement arg0 { get; }
        public AElement arg1 { get; }
        public AElement arg2 { get; }
        public AElement arg3 { get; }

        public Instruction(JObject obj, Dictionary<UInt32, JToken> objects)
        {
            op = (string)obj["op"];
            arg0 = AElement.Create(obj["arg0"], objects);
            arg1 = AElement.Create(obj["arg1"], objects);
            arg2 = AElement.Create(obj["arg2"], objects);
            arg3 = AElement.Create(obj["arg3"], objects);
        }

        public override string Description() => null;
    }

    class Null : AElement
    {
        public Null() {}

        public override string Description() => "null";
    }

    class Boolean : AElement
    {
        bool value;

        public Boolean(JToken obj)
        {
            value = (bool)obj;
        }

        public override string Description() => value.ToString();
    }

    class Integer : AElement
    {
        long value;

        public Integer(JToken obj)
        {
            value = (long)obj;
        }

        public override string Description() => value.ToString();
    }

    class Float : AElement
    {
        float value;

        public Float(JToken obj)
        {
            value = (float)obj;
        }

        public override string Description() => value.ToString();
    }

    class String : AElement
    {
        string value;

        public String(JToken obj)
        {
            value = (string)obj;
        }

        public override string Description() => '"' + value + '"';
    }

    abstract class AObject : AElement
    {
        const string head = "POINTER:";

        public static bool IsAddr(string str)
        {
            return str.Length > head.Length && str.Substring(0, head.Length).Equals(head);
        }

        public static UInt32 StrToAddr(string str)
        {
            if (IsAddr(str) == false)
                return 0;
            str = str.Substring(head.Length);
            return UInt32.Parse(str, System.Globalization.NumberStyles.HexNumber);
        }
    }

    class JsonObject : AObject
    {
        Dictionary<string, AElement> elems;
        string ObjectType;

        public JsonObject(JToken obj, Dictionary<UInt32, JToken> objects)
        {
            elems = new Dictionary<string, AElement>();
            foreach (var it in (JObject)obj)
            {
                if (it.Key == "ObjectType")
                    ObjectType = (string)it.Value;
                else
                    elems[it.Key] = AElement.Create(it.Value, objects);
            }
        }

        public override string Description() => "{...}";

        public override void AddChildsToTree(ItemCollection items, List<AElement> stack)
        {
            if (stack.Contains(this))
                return;
            stack.Add(this);
            foreach (var it in elems)
            {
                TreeViewItem item = new TreeViewItem() { Header = it.Key };
                it.Value.AddChildsToTree(item.Items, stack);
                items.Add(item);
            }
            stack.Remove(this);
        }
    }

    class Reference : AObject
    {
        static List<AElement> stack = new List<AElement>();

        JToken token;
        AElement target;

        public Reference(JToken obj, Dictionary<UInt32, JToken> objects)
        {
            UInt32 addr = AObject.StrToAddr((string)obj);
            if (objects.TryGetValue(addr, out token) == false)
                target = AObject.Create(token, objects);
            else
            {
                Console.WriteLine("Unknown object at address " + addr);
                token = null;
            }
            target = null;
        }

        private void Load()
        {
            if (target != null)
                return;
        }

        public override string Description()
        {
            return target.Description();
        }

        public override void AddChildsToTree(ItemCollection items, List<AElement> stack)
        {
            if (stack.Contains(this))
                return;
            target.AddChildsToTree(items, stack);
            stack.Remove(this);
        }
    }

    class JsonArray : AObject
    {
        List<AElement> elems;

        public JsonArray(JToken obj, Dictionary<UInt32, JToken> objects)
        {
            elems = new List<AElement>();
            foreach (var it in (JArray)obj)
            {
                elems.Add(AElement.Create(it, objects));
            }
        }

        public override string Description() => "[...]";

        public override void AddChildsToTree(ItemCollection items, List<AElement> stack)
        {
            if (stack.Contains(this))
                return;
            stack.Add(this);
            foreach (AElement it in elems)
            {
                TreeViewItem item = new TreeViewItem() { Header = it.GetTreeLabel() };
                it.AddChildsToTree(item.Items, stack);
                items.Add(item);
            }
            stack.Remove(this);
        }
    }
}
