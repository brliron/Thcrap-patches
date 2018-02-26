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

        public virtual void AddChildsToTree(ItemCollection items)
        { }

        public virtual void Load(Dictionary<UInt32, AElement> objects)
        { }

        public static AElement Create(JToken obj)
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
                        return new Reference(obj);
                    else
                        return new String(obj);

                case JTokenType.Object:
                    return new JsonObject(obj);

                case JTokenType.Array:
                    return new JsonArray(obj);

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

        public Instruction(JObject obj)
        {
            op = (string)obj["op"];
            arg0 = AElement.Create(obj["arg0"]);
            arg1 = AElement.Create(obj["arg1"]);
            arg2 = AElement.Create(obj["arg2"]);
            arg3 = AElement.Create(obj["arg3"]);
        }

        public override string Description() => null;

        public override void Load(Dictionary<UInt32, AElement> objects)
        {
            arg0.Load(objects);
            arg1.Load(objects);
            arg2.Load(objects);
            arg3.Load(objects);
        }
    }

    class Null : AElement
    {
        public Null()
        { }

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
        Dictionary<AElement, AElement> elems;
        string ObjectType;

        public JsonObject(JToken obj)
        {
            elems = new Dictionary<AElement, AElement>();
            foreach (var it in (JObject)obj)
            {
                if (it.Key == "ObjectType")
                    ObjectType = (string)it.Value;
                else
                    elems[AElement.Create(it.Key)] = AElement.Create(it.Value);
            }
        }

        public override string Description() => "{...}";

        public override void Load(Dictionary<UInt32, AElement> objects)
        {
            foreach (var it in elems)
            {
                it.Key.Load(objects);
                it.Value.Load(objects);
            }
        }

        public override void AddChildsToTree(ItemCollection items)
        {
            foreach (var it in elems)
            {
                TreeViewItem item = new TreeViewItem() { Header = it.Key };
                // TODO: try it.Key.AddChildsToTree(), create a new node if it added elements
                it.Value.AddChildsToTree(item.Items);
                items.Add(item);
            }
        }
    }

    class Reference : AObject
    {
        UInt32 addr;
        AElement target;
        bool isRecursing;

        public Reference(JToken obj)
        {
            addr = AObject.StrToAddr((string)obj);
            target = null;
            isRecursing = false;
        }

        public override void Load(Dictionary<UInt32, AElement> objects)
        {
            if (target != null)
                return;

            if (objects.TryGetValue(addr, out target))
                target.Load(objects);
            else
            {
                Console.WriteLine("Unknown object at address " + addr);
                target = null;
            }
        }

        public override string Description()
        {
            if (target == null)
                return "<ref not loaded>";
            return target.Description();
        }

        public override void AddChildsToTree(ItemCollection items)
        {
            if (target == null)
            {
                items.Add(new TreeViewItem() { Header = "<ref not loaded>" });
                return;
            }
            if (isRecursing)
                return; // Infinite recursion
            isRecursing = true;
            target.AddChildsToTree(items);
            isRecursing = false;
        }
    }

    class JsonArray : AObject
    {
        List<AElement> elems;

        public JsonArray(JToken obj)
        {
            elems = new List<AElement>();
            foreach (var it in (JArray)obj)
            {
                elems.Add(AElement.Create(it));
            }
        }

        public override string Description() => "[...]";

        public override void Load(Dictionary<UInt32, AElement> objects)
        {
            foreach (AElement it in elems)
                it.Load(objects);
        }

        public override void AddChildsToTree(ItemCollection items)
        {
            foreach (AElement it in elems)
            {
                TreeViewItem item = new TreeViewItem() { Header = it.GetTreeLabel() };
                it.AddChildsToTree(item.Items);
                items.Add(item);
            }
        }
    }
}
