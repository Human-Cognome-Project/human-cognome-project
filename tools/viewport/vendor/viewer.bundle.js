(()=>{var $0=Object.create;var cf=Object.defineProperty;var H0=Object.getOwnPropertyDescriptor;var G0=Object.getOwnPropertyNames;var W0=Object.getPrototypeOf,q0=Object.prototype.hasOwnProperty;var te=(n,e)=>()=>(e||n((e={exports:{}}).exports,e),e.exports);var X0=(n,e,t,i)=>{if(e&&typeof e=="object"||typeof e=="function")for(let r of G0(e))!q0.call(n,r)&&r!==t&&cf(n,r,{get:()=>e[r],enumerable:!(i=H0(e,r))||i.enumerable});return n};var j0=(n,e,t)=>(t=n!=null?$0(W0(n)):{},X0(e||!n||!n.__esModule?cf(t,"default",{value:n,enumerable:!0}):t,n));var la=te(nt=>{"use strict";Object.defineProperty(nt,"__esModule",{value:!0});nt.regexpCode=nt.getEsmExportName=nt.getProperty=nt.safeStringify=nt.stringify=nt.strConcat=nt.addCodeArg=nt.str=nt._=nt.nil=nt._Code=nt.Name=nt.IDENTIFIER=nt._CodeOrName=void 0;var oa=class{};nt._CodeOrName=oa;nt.IDENTIFIER=/^[a-z$_][a-z$_0-9]*$/i;var ur=class extends oa{constructor(e){if(super(),!nt.IDENTIFIER.test(e))throw new Error("CodeGen: name must be a valid identifier");this.str=e}toString(){return this.str}emptyStr(){return!1}get names(){return{[this.str]:1}}};nt.Name=ur;var An=class extends oa{constructor(e){super(),this._items=typeof e=="string"?[e]:e}toString(){return this.str}emptyStr(){if(this._items.length>1)return!1;let e=this._items[0];return e===""||e==='""'}get str(){var e;return(e=this._str)!==null&&e!==void 0?e:this._str=this._items.reduce((t,i)=>`${t}${i}`,"")}get names(){var e;return(e=this._names)!==null&&e!==void 0?e:this._names=this._items.reduce((t,i)=>(i instanceof ur&&(t[i.str]=(t[i.str]||0)+1),t),{})}};nt._Code=An;nt.nil=new An("");function Jp(n,...e){let t=[n[0]],i=0;for(;i<e.length;)Bu(t,e[i]),t.push(n[++i]);return new An(t)}nt._=Jp;var Fu=new An("+");function Qp(n,...e){let t=[ca(n[0])],i=0;for(;i<e.length;)t.push(Fu),Bu(t,e[i]),t.push(Fu,ca(n[++i]));return vM(t),new An(t)}nt.str=Qp;function Bu(n,e){e instanceof An?n.push(...e._items):e instanceof ur?n.push(e):n.push(bM(e))}nt.addCodeArg=Bu;function vM(n){let e=1;for(;e<n.length-1;){if(n[e]===Fu){let t=xM(n[e-1],n[e+1]);if(t!==void 0){n.splice(e-1,3,t);continue}n[e++]="+"}e++}}function xM(n,e){if(e==='""')return n;if(n==='""')return e;if(typeof n=="string")return e instanceof ur||n[n.length-1]!=='"'?void 0:typeof e!="string"?`${n.slice(0,-1)}${e}"`:e[0]==='"'?n.slice(0,-1)+e.slice(1):void 0;if(typeof e=="string"&&e[0]==='"'&&!(n instanceof ur))return`"${n}${e.slice(1)}`}function SM(n,e){return e.emptyStr()?n:n.emptyStr()?e:Qp`${n}${e}`}nt.strConcat=SM;function bM(n){return typeof n=="number"||typeof n=="boolean"||n===null?n:ca(Array.isArray(n)?n.join(","):n)}function MM(n){return new An(ca(n))}nt.stringify=MM;function ca(n){return JSON.stringify(n).replace(/\u2028/g,"\\u2028").replace(/\u2029/g,"\\u2029")}nt.safeStringify=ca;function EM(n){return typeof n=="string"&&nt.IDENTIFIER.test(n)?new An(`.${n}`):Jp`[${n}]`}nt.getProperty=EM;function wM(n){if(typeof n=="string"&&nt.IDENTIFIER.test(n))return new An(`${n}`);throw new Error(`CodeGen: invalid export name: ${n}, use explicit $id name mapping`)}nt.getEsmExportName=wM;function TM(n){return new An(n.toString())}nt.regexpCode=TM});var Vu=te(un=>{"use strict";Object.defineProperty(un,"__esModule",{value:!0});un.ValueScope=un.ValueScopeName=un.Scope=un.varKinds=un.UsedValueState=void 0;var ln=la(),ku=class extends Error{constructor(e){super(`CodeGen: "code" for ${e} not defined`),this.value=e.value}},Ic;(function(n){n[n.Started=0]="Started",n[n.Completed=1]="Completed"})(Ic||(un.UsedValueState=Ic={}));un.varKinds={const:new ln.Name("const"),let:new ln.Name("let"),var:new ln.Name("var")};var Dc=class{constructor({prefixes:e,parent:t}={}){this._names={},this._prefixes=e,this._parent=t}toName(e){return e instanceof ln.Name?e:this.name(e)}name(e){return new ln.Name(this._newName(e))}_newName(e){let t=this._names[e]||this._nameGroup(e);return`${e}${t.index++}`}_nameGroup(e){var t,i;if(!((i=(t=this._parent)===null||t===void 0?void 0:t._prefixes)===null||i===void 0)&&i.has(e)||this._prefixes&&!this._prefixes.has(e))throw new Error(`CodeGen: prefix "${e}" is not allowed in this scope`);return this._names[e]={prefix:e,index:0}}};un.Scope=Dc;var Nc=class extends ln.Name{constructor(e,t){super(t),this.prefix=e}setValue(e,{property:t,itemIndex:i}){this.value=e,this.scopePath=(0,ln._)`.${new ln.Name(t)}[${i}]`}};un.ValueScopeName=Nc;var AM=(0,ln._)`\n`,zu=class extends Dc{constructor(e){super(e),this._values={},this._scope=e.scope,this.opts={...e,_n:e.lines?AM:ln.nil}}get(){return this._scope}name(e){return new Nc(e,this._newName(e))}value(e,t){var i;if(t.ref===void 0)throw new Error("CodeGen: ref must be passed in value");let r=this.toName(e),{prefix:s}=r,a=(i=t.key)!==null&&i!==void 0?i:t.ref,o=this._values[s];if(o){let l=o.get(a);if(l)return l}else o=this._values[s]=new Map;o.set(a,r);let c=this._scope[s]||(this._scope[s]=[]),u=c.length;return c[u]=t.ref,r.setValue(t,{property:s,itemIndex:u}),r}getValue(e,t){let i=this._values[e];if(i)return i.get(t)}scopeRefs(e,t=this._values){return this._reduceValues(t,i=>{if(i.scopePath===void 0)throw new Error(`CodeGen: name "${i}" has no value`);return(0,ln._)`${e}${i.scopePath}`})}scopeCode(e=this._values,t,i){return this._reduceValues(e,r=>{if(r.value===void 0)throw new Error(`CodeGen: name "${r}" has no value`);return r.value.code},t,i)}_reduceValues(e,t,i={},r){let s=ln.nil;for(let a in e){let o=e[a];if(!o)continue;let c=i[a]=i[a]||new Map;o.forEach(u=>{if(c.has(u))return;c.set(u,Ic.Started);let l=t(u);if(l){let h=this.opts.es5?un.varKinds.var:un.varKinds.const;s=(0,ln._)`${s}${h} ${u} = ${l};${this.opts._n}`}else if(l=r?.(u))s=(0,ln._)`${s}${l}${this.opts._n}`;else throw new ku(u);c.set(u,Ic.Completed)})}return s}};un.ValueScope=zu});var Be=te(Ge=>{"use strict";Object.defineProperty(Ge,"__esModule",{value:!0});Ge.or=Ge.and=Ge.not=Ge.CodeGen=Ge.operators=Ge.varKinds=Ge.ValueScopeName=Ge.ValueScope=Ge.Scope=Ge.Name=Ge.regexpCode=Ge.stringify=Ge.getProperty=Ge.nil=Ge.strConcat=Ge.str=Ge._=void 0;var Ke=la(),kn=Vu(),Li=la();Object.defineProperty(Ge,"_",{enumerable:!0,get:function(){return Li._}});Object.defineProperty(Ge,"str",{enumerable:!0,get:function(){return Li.str}});Object.defineProperty(Ge,"strConcat",{enumerable:!0,get:function(){return Li.strConcat}});Object.defineProperty(Ge,"nil",{enumerable:!0,get:function(){return Li.nil}});Object.defineProperty(Ge,"getProperty",{enumerable:!0,get:function(){return Li.getProperty}});Object.defineProperty(Ge,"stringify",{enumerable:!0,get:function(){return Li.stringify}});Object.defineProperty(Ge,"regexpCode",{enumerable:!0,get:function(){return Li.regexpCode}});Object.defineProperty(Ge,"Name",{enumerable:!0,get:function(){return Li.Name}});var Fc=Vu();Object.defineProperty(Ge,"Scope",{enumerable:!0,get:function(){return Fc.Scope}});Object.defineProperty(Ge,"ValueScope",{enumerable:!0,get:function(){return Fc.ValueScope}});Object.defineProperty(Ge,"ValueScopeName",{enumerable:!0,get:function(){return Fc.ValueScopeName}});Object.defineProperty(Ge,"varKinds",{enumerable:!0,get:function(){return Fc.varKinds}});Ge.operators={GT:new Ke._Code(">"),GTE:new Ke._Code(">="),LT:new Ke._Code("<"),LTE:new Ke._Code("<="),EQ:new Ke._Code("==="),NEQ:new Ke._Code("!=="),NOT:new Ke._Code("!"),OR:new Ke._Code("||"),AND:new Ke._Code("&&"),ADD:new Ke._Code("+")};var fi=class{optimizeNodes(){return this}optimizeNames(e,t){return this}},$u=class extends fi{constructor(e,t,i){super(),this.varKind=e,this.name=t,this.rhs=i}render({es5:e,_n:t}){let i=e?kn.varKinds.var:this.varKind,r=this.rhs===void 0?"":` = ${this.rhs}`;return`${i} ${this.name}${r};`+t}optimizeNames(e,t){if(e[this.name.str])return this.rhs&&(this.rhs=cs(this.rhs,e,t)),this}get names(){return this.rhs instanceof Ke._CodeOrName?this.rhs.names:{}}},Lc=class extends fi{constructor(e,t,i){super(),this.lhs=e,this.rhs=t,this.sideEffects=i}render({_n:e}){return`${this.lhs} = ${this.rhs};`+e}optimizeNames(e,t){if(!(this.lhs instanceof Ke.Name&&!e[this.lhs.str]&&!this.sideEffects))return this.rhs=cs(this.rhs,e,t),this}get names(){let e=this.lhs instanceof Ke.Name?{}:{...this.lhs.names};return Oc(e,this.rhs)}},Hu=class extends Lc{constructor(e,t,i,r){super(e,i,r),this.op=t}render({_n:e}){return`${this.lhs} ${this.op}= ${this.rhs};`+e}},Gu=class extends fi{constructor(e){super(),this.label=e,this.names={}}render({_n:e}){return`${this.label}:`+e}},Wu=class extends fi{constructor(e){super(),this.label=e,this.names={}}render({_n:e}){return`break${this.label?` ${this.label}`:""};`+e}},qu=class extends fi{constructor(e){super(),this.error=e}render({_n:e}){return`throw ${this.error};`+e}get names(){return this.error.names}},Xu=class extends fi{constructor(e){super(),this.code=e}render({_n:e}){return`${this.code};`+e}optimizeNodes(){return`${this.code}`?this:void 0}optimizeNames(e,t){return this.code=cs(this.code,e,t),this}get names(){return this.code instanceof Ke._CodeOrName?this.code.names:{}}},ua=class extends fi{constructor(e=[]){super(),this.nodes=e}render(e){return this.nodes.reduce((t,i)=>t+i.render(e),"")}optimizeNodes(){let{nodes:e}=this,t=e.length;for(;t--;){let i=e[t].optimizeNodes();Array.isArray(i)?e.splice(t,1,...i):i?e[t]=i:e.splice(t,1)}return e.length>0?this:void 0}optimizeNames(e,t){let{nodes:i}=this,r=i.length;for(;r--;){let s=i[r];s.optimizeNames(e,t)||(RM(e,s.names),i.splice(r,1))}return i.length>0?this:void 0}get names(){return this.nodes.reduce((e,t)=>fr(e,t.names),{})}},pi=class extends ua{render(e){return"{"+e._n+super.render(e)+"}"+e._n}},ju=class extends ua{},os=class extends pi{};os.kind="else";var hr=class n extends pi{constructor(e,t){super(t),this.condition=e}render(e){let t=`if(${this.condition})`+super.render(e);return this.else&&(t+="else "+this.else.render(e)),t}optimizeNodes(){super.optimizeNodes();let e=this.condition;if(e===!0)return this.nodes;let t=this.else;if(t){let i=t.optimizeNodes();t=this.else=Array.isArray(i)?new os(i):i}if(t)return e===!1?t instanceof n?t:t.nodes:this.nodes.length?this:new n(em(e),t instanceof n?[t]:t.nodes);if(!(e===!1||!this.nodes.length))return this}optimizeNames(e,t){var i;if(this.else=(i=this.else)===null||i===void 0?void 0:i.optimizeNames(e,t),!!(super.optimizeNames(e,t)||this.else))return this.condition=cs(this.condition,e,t),this}get names(){let e=super.names;return Oc(e,this.condition),this.else&&fr(e,this.else.names),e}};hr.kind="if";var dr=class extends pi{};dr.kind="for";var Yu=class extends dr{constructor(e){super(),this.iteration=e}render(e){return`for(${this.iteration})`+super.render(e)}optimizeNames(e,t){if(super.optimizeNames(e,t))return this.iteration=cs(this.iteration,e,t),this}get names(){return fr(super.names,this.iteration.names)}},Zu=class extends dr{constructor(e,t,i,r){super(),this.varKind=e,this.name=t,this.from=i,this.to=r}render(e){let t=e.es5?kn.varKinds.var:this.varKind,{name:i,from:r,to:s}=this;return`for(${t} ${i}=${r}; ${i}<${s}; ${i}++)`+super.render(e)}get names(){let e=Oc(super.names,this.from);return Oc(e,this.to)}},Uc=class extends dr{constructor(e,t,i,r){super(),this.loop=e,this.varKind=t,this.name=i,this.iterable=r}render(e){return`for(${this.varKind} ${this.name} ${this.loop} ${this.iterable})`+super.render(e)}optimizeNames(e,t){if(super.optimizeNames(e,t))return this.iterable=cs(this.iterable,e,t),this}get names(){return fr(super.names,this.iterable.names)}},ha=class extends pi{constructor(e,t,i){super(),this.name=e,this.args=t,this.async=i}render(e){return`${this.async?"async ":""}function ${this.name}(${this.args})`+super.render(e)}};ha.kind="func";var da=class extends ua{render(e){return"return "+super.render(e)}};da.kind="return";var Ku=class extends pi{render(e){let t="try"+super.render(e);return this.catch&&(t+=this.catch.render(e)),this.finally&&(t+=this.finally.render(e)),t}optimizeNodes(){var e,t;return super.optimizeNodes(),(e=this.catch)===null||e===void 0||e.optimizeNodes(),(t=this.finally)===null||t===void 0||t.optimizeNodes(),this}optimizeNames(e,t){var i,r;return super.optimizeNames(e,t),(i=this.catch)===null||i===void 0||i.optimizeNames(e,t),(r=this.finally)===null||r===void 0||r.optimizeNames(e,t),this}get names(){let e=super.names;return this.catch&&fr(e,this.catch.names),this.finally&&fr(e,this.finally.names),e}},fa=class extends pi{constructor(e){super(),this.error=e}render(e){return`catch(${this.error})`+super.render(e)}};fa.kind="catch";var pa=class extends pi{render(e){return"finally"+super.render(e)}};pa.kind="finally";var Ju=class{constructor(e,t={}){this._values={},this._blockStarts=[],this._constants={},this.opts={...t,_n:t.lines?`
`:""},this._extScope=e,this._scope=new kn.Scope({parent:e}),this._nodes=[new ju]}toString(){return this._root.render(this.opts)}name(e){return this._scope.name(e)}scopeName(e){return this._extScope.name(e)}scopeValue(e,t){let i=this._extScope.value(e,t);return(this._values[i.prefix]||(this._values[i.prefix]=new Set)).add(i),i}getScopeValue(e,t){return this._extScope.getValue(e,t)}scopeRefs(e){return this._extScope.scopeRefs(e,this._values)}scopeCode(){return this._extScope.scopeCode(this._values)}_def(e,t,i,r){let s=this._scope.toName(t);return i!==void 0&&r&&(this._constants[s.str]=i),this._leafNode(new $u(e,s,i)),s}const(e,t,i){return this._def(kn.varKinds.const,e,t,i)}let(e,t,i){return this._def(kn.varKinds.let,e,t,i)}var(e,t,i){return this._def(kn.varKinds.var,e,t,i)}assign(e,t,i){return this._leafNode(new Lc(e,t,i))}add(e,t){return this._leafNode(new Hu(e,Ge.operators.ADD,t))}code(e){return typeof e=="function"?e():e!==Ke.nil&&this._leafNode(new Xu(e)),this}object(...e){let t=["{"];for(let[i,r]of e)t.length>1&&t.push(","),t.push(i),(i!==r||this.opts.es5)&&(t.push(":"),(0,Ke.addCodeArg)(t,r));return t.push("}"),new Ke._Code(t)}if(e,t,i){if(this._blockNode(new hr(e)),t&&i)this.code(t).else().code(i).endIf();else if(t)this.code(t).endIf();else if(i)throw new Error('CodeGen: "else" body without "then" body');return this}elseIf(e){return this._elseNode(new hr(e))}else(){return this._elseNode(new os)}endIf(){return this._endBlockNode(hr,os)}_for(e,t){return this._blockNode(e),t&&this.code(t).endFor(),this}for(e,t){return this._for(new Yu(e),t)}forRange(e,t,i,r,s=this.opts.es5?kn.varKinds.var:kn.varKinds.let){let a=this._scope.toName(e);return this._for(new Zu(s,a,t,i),()=>r(a))}forOf(e,t,i,r=kn.varKinds.const){let s=this._scope.toName(e);if(this.opts.es5){let a=t instanceof Ke.Name?t:this.var("_arr",t);return this.forRange("_i",0,(0,Ke._)`${a}.length`,o=>{this.var(s,(0,Ke._)`${a}[${o}]`),i(s)})}return this._for(new Uc("of",r,s,t),()=>i(s))}forIn(e,t,i,r=this.opts.es5?kn.varKinds.var:kn.varKinds.const){if(this.opts.ownProperties)return this.forOf(e,(0,Ke._)`Object.keys(${t})`,i);let s=this._scope.toName(e);return this._for(new Uc("in",r,s,t),()=>i(s))}endFor(){return this._endBlockNode(dr)}label(e){return this._leafNode(new Gu(e))}break(e){return this._leafNode(new Wu(e))}return(e){let t=new da;if(this._blockNode(t),this.code(e),t.nodes.length!==1)throw new Error('CodeGen: "return" should have one node');return this._endBlockNode(da)}try(e,t,i){if(!t&&!i)throw new Error('CodeGen: "try" without "catch" and "finally"');let r=new Ku;if(this._blockNode(r),this.code(e),t){let s=this.name("e");this._currNode=r.catch=new fa(s),t(s)}return i&&(this._currNode=r.finally=new pa,this.code(i)),this._endBlockNode(fa,pa)}throw(e){return this._leafNode(new qu(e))}block(e,t){return this._blockStarts.push(this._nodes.length),e&&this.code(e).endBlock(t),this}endBlock(e){let t=this._blockStarts.pop();if(t===void 0)throw new Error("CodeGen: not in self-balancing block");let i=this._nodes.length-t;if(i<0||e!==void 0&&i!==e)throw new Error(`CodeGen: wrong number of nodes: ${i} vs ${e} expected`);return this._nodes.length=t,this}func(e,t=Ke.nil,i,r){return this._blockNode(new ha(e,t,i)),r&&this.code(r).endFunc(),this}endFunc(){return this._endBlockNode(ha)}optimize(e=1){for(;e-- >0;)this._root.optimizeNodes(),this._root.optimizeNames(this._root.names,this._constants)}_leafNode(e){return this._currNode.nodes.push(e),this}_blockNode(e){this._currNode.nodes.push(e),this._nodes.push(e)}_endBlockNode(e,t){let i=this._currNode;if(i instanceof e||t&&i instanceof t)return this._nodes.pop(),this;throw new Error(`CodeGen: not in block "${t?`${e.kind}/${t.kind}`:e.kind}"`)}_elseNode(e){let t=this._currNode;if(!(t instanceof hr))throw new Error('CodeGen: "else" without "if"');return this._currNode=t.else=e,this}get _root(){return this._nodes[0]}get _currNode(){let e=this._nodes;return e[e.length-1]}set _currNode(e){let t=this._nodes;t[t.length-1]=e}};Ge.CodeGen=Ju;function fr(n,e){for(let t in e)n[t]=(n[t]||0)+(e[t]||0);return n}function Oc(n,e){return e instanceof Ke._CodeOrName?fr(n,e.names):n}function cs(n,e,t){if(n instanceof Ke.Name)return i(n);if(!r(n))return n;return new Ke._Code(n._items.reduce((s,a)=>(a instanceof Ke.Name&&(a=i(a)),a instanceof Ke._Code?s.push(...a._items):s.push(a),s),[]));function i(s){let a=t[s.str];return a===void 0||e[s.str]!==1?s:(delete e[s.str],a)}function r(s){return s instanceof Ke._Code&&s._items.some(a=>a instanceof Ke.Name&&e[a.str]===1&&t[a.str]!==void 0)}}function RM(n,e){for(let t in e)n[t]=(n[t]||0)-(e[t]||0)}function em(n){return typeof n=="boolean"||typeof n=="number"||n===null?!n:(0,Ke._)`!${Qu(n)}`}Ge.not=em;var CM=tm(Ge.operators.AND);function PM(...n){return n.reduce(CM)}Ge.and=PM;var IM=tm(Ge.operators.OR);function DM(...n){return n.reduce(IM)}Ge.or=DM;function tm(n){return(e,t)=>e===Ke.nil?t:t===Ke.nil?e:(0,Ke._)`${Qu(e)} ${n} ${Qu(t)}`}function Qu(n){return n instanceof Ke.Name?n:(0,Ke._)`(${n})`}});var Xe=te(qe=>{"use strict";Object.defineProperty(qe,"__esModule",{value:!0});qe.checkStrictMode=qe.getErrorPath=qe.Type=qe.useFunc=qe.setEvaluated=qe.evaluatedPropsToName=qe.mergeEvaluated=qe.eachItem=qe.unescapeJsonPointer=qe.escapeJsonPointer=qe.escapeFragment=qe.unescapeFragment=qe.schemaRefOrVal=qe.schemaHasRulesButRef=qe.schemaHasRules=qe.checkUnknownRules=qe.alwaysValidSchema=qe.toHash=void 0;var dt=Be(),NM=la();function LM(n){let e={};for(let t of n)e[t]=!0;return e}qe.toHash=LM;function UM(n,e){return typeof e=="boolean"?e:Object.keys(e).length===0?!0:(rm(n,e),!sm(e,n.self.RULES.all))}qe.alwaysValidSchema=UM;function rm(n,e=n.schema){let{opts:t,self:i}=n;if(!t.strictSchema||typeof e=="boolean")return;let r=i.RULES.keywords;for(let s in e)r[s]||cm(n,`unknown keyword: "${s}"`)}qe.checkUnknownRules=rm;function sm(n,e){if(typeof n=="boolean")return!n;for(let t in n)if(e[t])return!0;return!1}qe.schemaHasRules=sm;function OM(n,e){if(typeof n=="boolean")return!n;for(let t in n)if(t!=="$ref"&&e.all[t])return!0;return!1}qe.schemaHasRulesButRef=OM;function FM({topSchemaRef:n,schemaPath:e},t,i,r){if(!r){if(typeof t=="number"||typeof t=="boolean")return t;if(typeof t=="string")return(0,dt._)`${t}`}return(0,dt._)`${n}${e}${(0,dt.getProperty)(i)}`}qe.schemaRefOrVal=FM;function BM(n){return am(decodeURIComponent(n))}qe.unescapeFragment=BM;function kM(n){return encodeURIComponent(th(n))}qe.escapeFragment=kM;function th(n){return typeof n=="number"?`${n}`:n.replace(/~/g,"~0").replace(/\//g,"~1")}qe.escapeJsonPointer=th;function am(n){return n.replace(/~1/g,"/").replace(/~0/g,"~")}qe.unescapeJsonPointer=am;function zM(n,e){if(Array.isArray(n))for(let t of n)e(t);else e(n)}qe.eachItem=zM;function nm({mergeNames:n,mergeToName:e,mergeValues:t,resultToName:i}){return(r,s,a,o)=>{let c=a===void 0?s:a instanceof dt.Name?(s instanceof dt.Name?n(r,s,a):e(r,s,a),a):s instanceof dt.Name?(e(r,a,s),s):t(s,a);return o===dt.Name&&!(c instanceof dt.Name)?i(r,c):c}}qe.mergeEvaluated={props:nm({mergeNames:(n,e,t)=>n.if((0,dt._)`${t} !== true && ${e} !== undefined`,()=>{n.if((0,dt._)`${e} === true`,()=>n.assign(t,!0),()=>n.assign(t,(0,dt._)`${t} || {}`).code((0,dt._)`Object.assign(${t}, ${e})`))}),mergeToName:(n,e,t)=>n.if((0,dt._)`${t} !== true`,()=>{e===!0?n.assign(t,!0):(n.assign(t,(0,dt._)`${t} || {}`),nh(n,t,e))}),mergeValues:(n,e)=>n===!0?!0:{...n,...e},resultToName:om}),items:nm({mergeNames:(n,e,t)=>n.if((0,dt._)`${t} !== true && ${e} !== undefined`,()=>n.assign(t,(0,dt._)`${e} === true ? true : ${t} > ${e} ? ${t} : ${e}`)),mergeToName:(n,e,t)=>n.if((0,dt._)`${t} !== true`,()=>n.assign(t,e===!0?!0:(0,dt._)`${t} > ${e} ? ${t} : ${e}`)),mergeValues:(n,e)=>n===!0?!0:Math.max(n,e),resultToName:(n,e)=>n.var("items",e)})};function om(n,e){if(e===!0)return n.var("props",!0);let t=n.var("props",(0,dt._)`{}`);return e!==void 0&&nh(n,t,e),t}qe.evaluatedPropsToName=om;function nh(n,e,t){Object.keys(t).forEach(i=>n.assign((0,dt._)`${e}${(0,dt.getProperty)(i)}`,!0))}qe.setEvaluated=nh;var im={};function VM(n,e){return n.scopeValue("func",{ref:e,code:im[e.code]||(im[e.code]=new NM._Code(e.code))})}qe.useFunc=VM;var eh;(function(n){n[n.Num=0]="Num",n[n.Str=1]="Str"})(eh||(qe.Type=eh={}));function $M(n,e,t){if(n instanceof dt.Name){let i=e===eh.Num;return t?i?(0,dt._)`"[" + ${n} + "]"`:(0,dt._)`"['" + ${n} + "']"`:i?(0,dt._)`"/" + ${n}`:(0,dt._)`"/" + ${n}.replace(/~/g, "~0").replace(/\\//g, "~1")`}return t?(0,dt.getProperty)(n).toString():"/"+th(n)}qe.getErrorPath=$M;function cm(n,e,t=n.opts.strictSchema){if(t){if(e=`strict mode: ${e}`,t===!0)throw new Error(e);n.self.logger.warn(e)}}qe.checkStrictMode=cm});var Rn=te(ih=>{"use strict";Object.defineProperty(ih,"__esModule",{value:!0});var Wt=Be(),HM={data:new Wt.Name("data"),valCxt:new Wt.Name("valCxt"),instancePath:new Wt.Name("instancePath"),parentData:new Wt.Name("parentData"),parentDataProperty:new Wt.Name("parentDataProperty"),rootData:new Wt.Name("rootData"),dynamicAnchors:new Wt.Name("dynamicAnchors"),vErrors:new Wt.Name("vErrors"),errors:new Wt.Name("errors"),this:new Wt.Name("this"),self:new Wt.Name("self"),scope:new Wt.Name("scope"),json:new Wt.Name("json"),jsonPos:new Wt.Name("jsonPos"),jsonLen:new Wt.Name("jsonLen"),jsonPart:new Wt.Name("jsonPart")};ih.default=HM});var ma=te(qt=>{"use strict";Object.defineProperty(qt,"__esModule",{value:!0});qt.extendErrors=qt.resetErrorsCount=qt.reportExtraError=qt.reportError=qt.keyword$DataError=qt.keywordError=void 0;var et=Be(),Bc=Xe(),Qt=Rn();qt.keywordError={message:({keyword:n})=>(0,et.str)`must pass "${n}" keyword validation`};qt.keyword$DataError={message:({keyword:n,schemaType:e})=>e?(0,et.str)`"${n}" keyword must be ${e} ($data)`:(0,et.str)`"${n}" keyword is invalid ($data)`};function GM(n,e=qt.keywordError,t,i){let{it:r}=n,{gen:s,compositeRule:a,allErrors:o}=r,c=hm(n,e,t);i??(a||o)?lm(s,c):um(r,(0,et._)`[${c}]`)}qt.reportError=GM;function WM(n,e=qt.keywordError,t){let{it:i}=n,{gen:r,compositeRule:s,allErrors:a}=i,o=hm(n,e,t);lm(r,o),s||a||um(i,Qt.default.vErrors)}qt.reportExtraError=WM;function qM(n,e){n.assign(Qt.default.errors,e),n.if((0,et._)`${Qt.default.vErrors} !== null`,()=>n.if(e,()=>n.assign((0,et._)`${Qt.default.vErrors}.length`,e),()=>n.assign(Qt.default.vErrors,null)))}qt.resetErrorsCount=qM;function XM({gen:n,keyword:e,schemaValue:t,data:i,errsCount:r,it:s}){if(r===void 0)throw new Error("ajv implementation error");let a=n.name("err");n.forRange("i",r,Qt.default.errors,o=>{n.const(a,(0,et._)`${Qt.default.vErrors}[${o}]`),n.if((0,et._)`${a}.instancePath === undefined`,()=>n.assign((0,et._)`${a}.instancePath`,(0,et.strConcat)(Qt.default.instancePath,s.errorPath))),n.assign((0,et._)`${a}.schemaPath`,(0,et.str)`${s.errSchemaPath}/${e}`),s.opts.verbose&&(n.assign((0,et._)`${a}.schema`,t),n.assign((0,et._)`${a}.data`,i))})}qt.extendErrors=XM;function lm(n,e){let t=n.const("err",e);n.if((0,et._)`${Qt.default.vErrors} === null`,()=>n.assign(Qt.default.vErrors,(0,et._)`[${t}]`),(0,et._)`${Qt.default.vErrors}.push(${t})`),n.code((0,et._)`${Qt.default.errors}++`)}function um(n,e){let{gen:t,validateName:i,schemaEnv:r}=n;r.$async?t.throw((0,et._)`new ${n.ValidationError}(${e})`):(t.assign((0,et._)`${i}.errors`,e),t.return(!1))}var pr={keyword:new et.Name("keyword"),schemaPath:new et.Name("schemaPath"),params:new et.Name("params"),propertyName:new et.Name("propertyName"),message:new et.Name("message"),schema:new et.Name("schema"),parentSchema:new et.Name("parentSchema")};function hm(n,e,t){let{createErrors:i}=n.it;return i===!1?(0,et._)`{}`:jM(n,e,t)}function jM(n,e,t={}){let{gen:i,it:r}=n,s=[YM(r,t),ZM(n,t)];return KM(n,e,s),i.object(...s)}function YM({errorPath:n},{instancePath:e}){let t=e?(0,et.str)`${n}${(0,Bc.getErrorPath)(e,Bc.Type.Str)}`:n;return[Qt.default.instancePath,(0,et.strConcat)(Qt.default.instancePath,t)]}function ZM({keyword:n,it:{errSchemaPath:e}},{schemaPath:t,parentSchema:i}){let r=i?e:(0,et.str)`${e}/${n}`;return t&&(r=(0,et.str)`${r}${(0,Bc.getErrorPath)(t,Bc.Type.Str)}`),[pr.schemaPath,r]}function KM(n,{params:e,message:t},i){let{keyword:r,data:s,schemaValue:a,it:o}=n,{opts:c,propertyName:u,topSchemaRef:l,schemaPath:h}=o;i.push([pr.keyword,r],[pr.params,typeof e=="function"?e(n):e||(0,et._)`{}`]),c.messages&&i.push([pr.message,typeof t=="function"?t(n):t]),c.verbose&&i.push([pr.schema,a],[pr.parentSchema,(0,et._)`${l}${h}`],[Qt.default.data,s]),u&&i.push([pr.propertyName,u])}});var fm=te(ls=>{"use strict";Object.defineProperty(ls,"__esModule",{value:!0});ls.boolOrEmptySchema=ls.topBoolOrEmptySchema=void 0;var JM=ma(),QM=Be(),eE=Rn(),tE={message:"boolean schema is false"};function nE(n){let{gen:e,schema:t,validateName:i}=n;t===!1?dm(n,!1):typeof t=="object"&&t.$async===!0?e.return(eE.default.data):(e.assign((0,QM._)`${i}.errors`,null),e.return(!0))}ls.topBoolOrEmptySchema=nE;function iE(n,e){let{gen:t,schema:i}=n;i===!1?(t.var(e,!1),dm(n)):t.var(e,!0)}ls.boolOrEmptySchema=iE;function dm(n,e){let{gen:t,data:i}=n,r={gen:t,keyword:"false schema",data:i,schema:!1,schemaCode:!1,schemaValue:!1,params:{},it:n};(0,JM.reportError)(r,tE,void 0,e)}});var rh=te(us=>{"use strict";Object.defineProperty(us,"__esModule",{value:!0});us.getRules=us.isJSONType=void 0;var rE=["string","number","integer","boolean","null","object","array"],sE=new Set(rE);function aE(n){return typeof n=="string"&&sE.has(n)}us.isJSONType=aE;function oE(){let n={number:{type:"number",rules:[]},string:{type:"string",rules:[]},array:{type:"array",rules:[]},object:{type:"object",rules:[]}};return{types:{...n,integer:!0,boolean:!0,null:!0},rules:[{rules:[]},n.number,n.string,n.array,n.object],post:{rules:[]},all:{},keywords:{}}}us.getRules=oE});var sh=te(Ui=>{"use strict";Object.defineProperty(Ui,"__esModule",{value:!0});Ui.shouldUseRule=Ui.shouldUseGroup=Ui.schemaHasRulesForType=void 0;function cE({schema:n,self:e},t){let i=e.RULES.types[t];return i&&i!==!0&&pm(n,i)}Ui.schemaHasRulesForType=cE;function pm(n,e){return e.rules.some(t=>mm(n,t))}Ui.shouldUseGroup=pm;function mm(n,e){var t;return n[e.keyword]!==void 0||((t=e.definition.implements)===null||t===void 0?void 0:t.some(i=>n[i]!==void 0))}Ui.shouldUseRule=mm});var ga=te(Xt=>{"use strict";Object.defineProperty(Xt,"__esModule",{value:!0});Xt.reportTypeError=Xt.checkDataTypes=Xt.checkDataType=Xt.coerceAndCheckDataType=Xt.getJSONTypes=Xt.getSchemaTypes=Xt.DataType=void 0;var lE=rh(),uE=sh(),hE=ma(),ze=Be(),gm=Xe(),hs;(function(n){n[n.Correct=0]="Correct",n[n.Wrong=1]="Wrong"})(hs||(Xt.DataType=hs={}));function dE(n){let e=_m(n.type);if(e.includes("null")){if(n.nullable===!1)throw new Error("type: null contradicts nullable: false")}else{if(!e.length&&n.nullable!==void 0)throw new Error('"nullable" cannot be used without "type"');n.nullable===!0&&e.push("null")}return e}Xt.getSchemaTypes=dE;function _m(n){let e=Array.isArray(n)?n:n?[n]:[];if(e.every(lE.isJSONType))return e;throw new Error("type must be JSONType or JSONType[]: "+e.join(","))}Xt.getJSONTypes=_m;function fE(n,e){let{gen:t,data:i,opts:r}=n,s=pE(e,r.coerceTypes),a=e.length>0&&!(s.length===0&&e.length===1&&(0,uE.schemaHasRulesForType)(n,e[0]));if(a){let o=oh(e,i,r.strictNumbers,hs.Wrong);t.if(o,()=>{s.length?mE(n,e,s):ch(n)})}return a}Xt.coerceAndCheckDataType=fE;var ym=new Set(["string","number","integer","boolean","null"]);function pE(n,e){return e?n.filter(t=>ym.has(t)||e==="array"&&t==="array"):[]}function mE(n,e,t){let{gen:i,data:r,opts:s}=n,a=i.let("dataType",(0,ze._)`typeof ${r}`),o=i.let("coerced",(0,ze._)`undefined`);s.coerceTypes==="array"&&i.if((0,ze._)`${a} == 'object' && Array.isArray(${r}) && ${r}.length == 1`,()=>i.assign(r,(0,ze._)`${r}[0]`).assign(a,(0,ze._)`typeof ${r}`).if(oh(e,r,s.strictNumbers),()=>i.assign(o,r))),i.if((0,ze._)`${o} !== undefined`);for(let u of t)(ym.has(u)||u==="array"&&s.coerceTypes==="array")&&c(u);i.else(),ch(n),i.endIf(),i.if((0,ze._)`${o} !== undefined`,()=>{i.assign(r,o),gE(n,o)});function c(u){switch(u){case"string":i.elseIf((0,ze._)`${a} == "number" || ${a} == "boolean"`).assign(o,(0,ze._)`"" + ${r}`).elseIf((0,ze._)`${r} === null`).assign(o,(0,ze._)`""`);return;case"number":i.elseIf((0,ze._)`${a} == "boolean" || ${r} === null
              || (${a} == "string" && ${r} && ${r} == +${r})`).assign(o,(0,ze._)`+${r}`);return;case"integer":i.elseIf((0,ze._)`${a} === "boolean" || ${r} === null
              || (${a} === "string" && ${r} && ${r} == +${r} && !(${r} % 1))`).assign(o,(0,ze._)`+${r}`);return;case"boolean":i.elseIf((0,ze._)`${r} === "false" || ${r} === 0 || ${r} === null`).assign(o,!1).elseIf((0,ze._)`${r} === "true" || ${r} === 1`).assign(o,!0);return;case"null":i.elseIf((0,ze._)`${r} === "" || ${r} === 0 || ${r} === false`),i.assign(o,null);return;case"array":i.elseIf((0,ze._)`${a} === "string" || ${a} === "number"
              || ${a} === "boolean" || ${r} === null`).assign(o,(0,ze._)`[${r}]`)}}}function gE({gen:n,parentData:e,parentDataProperty:t},i){n.if((0,ze._)`${e} !== undefined`,()=>n.assign((0,ze._)`${e}[${t}]`,i))}function ah(n,e,t,i=hs.Correct){let r=i===hs.Correct?ze.operators.EQ:ze.operators.NEQ,s;switch(n){case"null":return(0,ze._)`${e} ${r} null`;case"array":s=(0,ze._)`Array.isArray(${e})`;break;case"object":s=(0,ze._)`${e} && typeof ${e} == "object" && !Array.isArray(${e})`;break;case"integer":s=a((0,ze._)`!(${e} % 1) && !isNaN(${e})`);break;case"number":s=a();break;default:return(0,ze._)`typeof ${e} ${r} ${n}`}return i===hs.Correct?s:(0,ze.not)(s);function a(o=ze.nil){return(0,ze.and)((0,ze._)`typeof ${e} == "number"`,o,t?(0,ze._)`isFinite(${e})`:ze.nil)}}Xt.checkDataType=ah;function oh(n,e,t,i){if(n.length===1)return ah(n[0],e,t,i);let r,s=(0,gm.toHash)(n);if(s.array&&s.object){let a=(0,ze._)`typeof ${e} != "object"`;r=s.null?a:(0,ze._)`!${e} || ${a}`,delete s.null,delete s.array,delete s.object}else r=ze.nil;s.number&&delete s.integer;for(let a in s)r=(0,ze.and)(r,ah(a,e,t,i));return r}Xt.checkDataTypes=oh;var _E={message:({schema:n})=>`must be ${n}`,params:({schema:n,schemaValue:e})=>typeof n=="string"?(0,ze._)`{type: ${n}}`:(0,ze._)`{type: ${e}}`};function ch(n){let e=yE(n);(0,hE.reportError)(e,_E)}Xt.reportTypeError=ch;function yE(n){let{gen:e,data:t,schema:i}=n,r=(0,gm.schemaRefOrVal)(n,i,"type");return{gen:e,keyword:"type",data:t,schema:i.type,schemaCode:r,schemaValue:r,parentSchema:i,params:{},it:n}}});var xm=te(kc=>{"use strict";Object.defineProperty(kc,"__esModule",{value:!0});kc.assignDefaults=void 0;var ds=Be(),vE=Xe();function xE(n,e){let{properties:t,items:i}=n.schema;if(e==="object"&&t)for(let r in t)vm(n,r,t[r].default);else e==="array"&&Array.isArray(i)&&i.forEach((r,s)=>vm(n,s,r.default))}kc.assignDefaults=xE;function vm(n,e,t){let{gen:i,compositeRule:r,data:s,opts:a}=n;if(t===void 0)return;let o=(0,ds._)`${s}${(0,ds.getProperty)(e)}`;if(r){(0,vE.checkStrictMode)(n,`default is ignored for: ${o}`);return}let c=(0,ds._)`${o} === undefined`;a.useDefaults==="empty"&&(c=(0,ds._)`${c} || ${o} === null || ${o} === ""`),i.if(c,(0,ds._)`${o} = ${(0,ds.stringify)(t)}`)}});var Cn=te(lt=>{"use strict";Object.defineProperty(lt,"__esModule",{value:!0});lt.validateUnion=lt.validateArray=lt.usePattern=lt.callValidateCode=lt.schemaProperties=lt.allSchemaProperties=lt.noPropertyInData=lt.propertyInData=lt.isOwnProperty=lt.hasPropFunc=lt.reportMissingProp=lt.checkMissingProp=lt.checkReportMissingProp=void 0;var yt=Be(),lh=Xe(),Oi=Rn(),SE=Xe();function bE(n,e){let{gen:t,data:i,it:r}=n;t.if(hh(t,i,e,r.opts.ownProperties),()=>{n.setParams({missingProperty:(0,yt._)`${e}`},!0),n.error()})}lt.checkReportMissingProp=bE;function ME({gen:n,data:e,it:{opts:t}},i,r){return(0,yt.or)(...i.map(s=>(0,yt.and)(hh(n,e,s,t.ownProperties),(0,yt._)`${r} = ${s}`)))}lt.checkMissingProp=ME;function EE(n,e){n.setParams({missingProperty:e},!0),n.error()}lt.reportMissingProp=EE;function Sm(n){return n.scopeValue("func",{ref:Object.prototype.hasOwnProperty,code:(0,yt._)`Object.prototype.hasOwnProperty`})}lt.hasPropFunc=Sm;function uh(n,e,t){return(0,yt._)`${Sm(n)}.call(${e}, ${t})`}lt.isOwnProperty=uh;function wE(n,e,t,i){let r=(0,yt._)`${e}${(0,yt.getProperty)(t)} !== undefined`;return i?(0,yt._)`${r} && ${uh(n,e,t)}`:r}lt.propertyInData=wE;function hh(n,e,t,i){let r=(0,yt._)`${e}${(0,yt.getProperty)(t)} === undefined`;return i?(0,yt.or)(r,(0,yt.not)(uh(n,e,t))):r}lt.noPropertyInData=hh;function bm(n){return n?Object.keys(n).filter(e=>e!=="__proto__"):[]}lt.allSchemaProperties=bm;function TE(n,e){return bm(e).filter(t=>!(0,lh.alwaysValidSchema)(n,e[t]))}lt.schemaProperties=TE;function AE({schemaCode:n,data:e,it:{gen:t,topSchemaRef:i,schemaPath:r,errorPath:s},it:a},o,c,u){let l=u?(0,yt._)`${n}, ${e}, ${i}${r}`:e,h=[[Oi.default.instancePath,(0,yt.strConcat)(Oi.default.instancePath,s)],[Oi.default.parentData,a.parentData],[Oi.default.parentDataProperty,a.parentDataProperty],[Oi.default.rootData,Oi.default.rootData]];a.opts.dynamicRef&&h.push([Oi.default.dynamicAnchors,Oi.default.dynamicAnchors]);let f=(0,yt._)`${l}, ${t.object(...h)}`;return c!==yt.nil?(0,yt._)`${o}.call(${c}, ${f})`:(0,yt._)`${o}(${f})`}lt.callValidateCode=AE;var RE=(0,yt._)`new RegExp`;function CE({gen:n,it:{opts:e}},t){let i=e.unicodeRegExp?"u":"",{regExp:r}=e.code,s=r(t,i);return n.scopeValue("pattern",{key:s.toString(),ref:s,code:(0,yt._)`${r.code==="new RegExp"?RE:(0,SE.useFunc)(n,r)}(${t}, ${i})`})}lt.usePattern=CE;function PE(n){let{gen:e,data:t,keyword:i,it:r}=n,s=e.name("valid");if(r.allErrors){let o=e.let("valid",!0);return a(()=>e.assign(o,!1)),o}return e.var(s,!0),a(()=>e.break()),s;function a(o){let c=e.const("len",(0,yt._)`${t}.length`);e.forRange("i",0,c,u=>{n.subschema({keyword:i,dataProp:u,dataPropType:lh.Type.Num},s),e.if((0,yt.not)(s),o)})}}lt.validateArray=PE;function IE(n){let{gen:e,schema:t,keyword:i,it:r}=n;if(!Array.isArray(t))throw new Error("ajv implementation error");if(t.some(c=>(0,lh.alwaysValidSchema)(r,c))&&!r.opts.unevaluated)return;let a=e.let("valid",!1),o=e.name("_valid");e.block(()=>t.forEach((c,u)=>{let l=n.subschema({keyword:i,schemaProp:u,compositeRule:!0},o);e.assign(a,(0,yt._)`${a} || ${o}`),n.mergeValidEvaluated(l,o)||e.if((0,yt.not)(a))})),n.result(a,()=>n.reset(),()=>n.error(!0))}lt.validateUnion=IE});var wm=te(Jn=>{"use strict";Object.defineProperty(Jn,"__esModule",{value:!0});Jn.validateKeywordUsage=Jn.validSchemaType=Jn.funcKeywordCode=Jn.macroKeywordCode=void 0;var en=Be(),mr=Rn(),DE=Cn(),NE=ma();function LE(n,e){let{gen:t,keyword:i,schema:r,parentSchema:s,it:a}=n,o=e.macro.call(a.self,r,s,a),c=Em(t,i,o);a.opts.validateSchema!==!1&&a.self.validateSchema(o,!0);let u=t.name("valid");n.subschema({schema:o,schemaPath:en.nil,errSchemaPath:`${a.errSchemaPath}/${i}`,topSchemaRef:c,compositeRule:!0},u),n.pass(u,()=>n.error(!0))}Jn.macroKeywordCode=LE;function UE(n,e){var t;let{gen:i,keyword:r,schema:s,parentSchema:a,$data:o,it:c}=n;FE(c,e);let u=!o&&e.compile?e.compile.call(c.self,s,a,c):e.validate,l=Em(i,r,u),h=i.let("valid");n.block$data(h,f),n.ok((t=e.valid)!==null&&t!==void 0?t:h);function f(){if(e.errors===!1)_(),e.modifying&&Mm(n),m(()=>n.error());else{let d=e.async?p():g();e.modifying&&Mm(n),m(()=>OE(n,d))}}function p(){let d=i.let("ruleErrs",null);return i.try(()=>_((0,en._)`await `),w=>i.assign(h,!1).if((0,en._)`${w} instanceof ${c.ValidationError}`,()=>i.assign(d,(0,en._)`${w}.errors`),()=>i.throw(w))),d}function g(){let d=(0,en._)`${l}.errors`;return i.assign(d,null),_(en.nil),d}function _(d=e.async?(0,en._)`await `:en.nil){let w=c.opts.passContext?mr.default.this:mr.default.self,M=!("compile"in e&&!o||e.schema===!1);i.assign(h,(0,en._)`${d}${(0,DE.callValidateCode)(n,l,w,M)}`,e.modifying)}function m(d){var w;i.if((0,en.not)((w=e.valid)!==null&&w!==void 0?w:h),d)}}Jn.funcKeywordCode=UE;function Mm(n){let{gen:e,data:t,it:i}=n;e.if(i.parentData,()=>e.assign(t,(0,en._)`${i.parentData}[${i.parentDataProperty}]`))}function OE(n,e){let{gen:t}=n;t.if((0,en._)`Array.isArray(${e})`,()=>{t.assign(mr.default.vErrors,(0,en._)`${mr.default.vErrors} === null ? ${e} : ${mr.default.vErrors}.concat(${e})`).assign(mr.default.errors,(0,en._)`${mr.default.vErrors}.length`),(0,NE.extendErrors)(n)},()=>n.error())}function FE({schemaEnv:n},e){if(e.async&&!n.$async)throw new Error("async keyword in sync schema")}function Em(n,e,t){if(t===void 0)throw new Error(`keyword "${e}" failed to compile`);return n.scopeValue("keyword",typeof t=="function"?{ref:t}:{ref:t,code:(0,en.stringify)(t)})}function BE(n,e,t=!1){return!e.length||e.some(i=>i==="array"?Array.isArray(n):i==="object"?n&&typeof n=="object"&&!Array.isArray(n):typeof n==i||t&&typeof n>"u")}Jn.validSchemaType=BE;function kE({schema:n,opts:e,self:t,errSchemaPath:i},r,s){if(Array.isArray(r.keyword)?!r.keyword.includes(s):r.keyword!==s)throw new Error("ajv implementation error");let a=r.dependencies;if(a?.some(o=>!Object.prototype.hasOwnProperty.call(n,o)))throw new Error(`parent schema must have dependencies of ${s}: ${a.join(",")}`);if(r.validateSchema&&!r.validateSchema(n[s])){let c=`keyword "${s}" value is invalid at path "${i}": `+t.errorsText(r.validateSchema.errors);if(e.validateSchema==="log")t.logger.error(c);else throw new Error(c)}}Jn.validateKeywordUsage=kE});var Am=te(Fi=>{"use strict";Object.defineProperty(Fi,"__esModule",{value:!0});Fi.extendSubschemaMode=Fi.extendSubschemaData=Fi.getSubschema=void 0;var Qn=Be(),Tm=Xe();function zE(n,{keyword:e,schemaProp:t,schema:i,schemaPath:r,errSchemaPath:s,topSchemaRef:a}){if(e!==void 0&&i!==void 0)throw new Error('both "keyword" and "schema" passed, only one allowed');if(e!==void 0){let o=n.schema[e];return t===void 0?{schema:o,schemaPath:(0,Qn._)`${n.schemaPath}${(0,Qn.getProperty)(e)}`,errSchemaPath:`${n.errSchemaPath}/${e}`}:{schema:o[t],schemaPath:(0,Qn._)`${n.schemaPath}${(0,Qn.getProperty)(e)}${(0,Qn.getProperty)(t)}`,errSchemaPath:`${n.errSchemaPath}/${e}/${(0,Tm.escapeFragment)(t)}`}}if(i!==void 0){if(r===void 0||s===void 0||a===void 0)throw new Error('"schemaPath", "errSchemaPath" and "topSchemaRef" are required with "schema"');return{schema:i,schemaPath:r,topSchemaRef:a,errSchemaPath:s}}throw new Error('either "keyword" or "schema" must be passed')}Fi.getSubschema=zE;function VE(n,e,{dataProp:t,dataPropType:i,data:r,dataTypes:s,propertyName:a}){if(r!==void 0&&t!==void 0)throw new Error('both "data" and "dataProp" passed, only one allowed');let{gen:o}=e;if(t!==void 0){let{errorPath:u,dataPathArr:l,opts:h}=e,f=o.let("data",(0,Qn._)`${e.data}${(0,Qn.getProperty)(t)}`,!0);c(f),n.errorPath=(0,Qn.str)`${u}${(0,Tm.getErrorPath)(t,i,h.jsPropertySyntax)}`,n.parentDataProperty=(0,Qn._)`${t}`,n.dataPathArr=[...l,n.parentDataProperty]}if(r!==void 0){let u=r instanceof Qn.Name?r:o.let("data",r,!0);c(u),a!==void 0&&(n.propertyName=a)}s&&(n.dataTypes=s);function c(u){n.data=u,n.dataLevel=e.dataLevel+1,n.dataTypes=[],e.definedProperties=new Set,n.parentData=e.data,n.dataNames=[...e.dataNames,u]}}Fi.extendSubschemaData=VE;function $E(n,{jtdDiscriminator:e,jtdMetadata:t,compositeRule:i,createErrors:r,allErrors:s}){i!==void 0&&(n.compositeRule=i),r!==void 0&&(n.createErrors=r),s!==void 0&&(n.allErrors=s),n.jtdDiscriminator=e,n.jtdMetadata=t}Fi.extendSubschemaMode=$E});var dh=te((v2,Rm)=>{"use strict";Rm.exports=function n(e,t){if(e===t)return!0;if(e&&t&&typeof e=="object"&&typeof t=="object"){if(e.constructor!==t.constructor)return!1;var i,r,s;if(Array.isArray(e)){if(i=e.length,i!=t.length)return!1;for(r=i;r--!==0;)if(!n(e[r],t[r]))return!1;return!0}if(e.constructor===RegExp)return e.source===t.source&&e.flags===t.flags;if(e.valueOf!==Object.prototype.valueOf)return e.valueOf()===t.valueOf();if(e.toString!==Object.prototype.toString)return e.toString()===t.toString();if(s=Object.keys(e),i=s.length,i!==Object.keys(t).length)return!1;for(r=i;r--!==0;)if(!Object.prototype.hasOwnProperty.call(t,s[r]))return!1;for(r=i;r--!==0;){var a=s[r];if(!n(e[a],t[a]))return!1}return!0}return e!==e&&t!==t}});var Pm=te((x2,Cm)=>{"use strict";var Bi=Cm.exports=function(n,e,t){typeof e=="function"&&(t=e,e={}),t=e.cb||t;var i=typeof t=="function"?t:t.pre||function(){},r=t.post||function(){};zc(e,i,r,n,"",n)};Bi.keywords={additionalItems:!0,items:!0,contains:!0,additionalProperties:!0,propertyNames:!0,not:!0,if:!0,then:!0,else:!0};Bi.arrayKeywords={items:!0,allOf:!0,anyOf:!0,oneOf:!0};Bi.propsKeywords={$defs:!0,definitions:!0,properties:!0,patternProperties:!0,dependencies:!0};Bi.skipKeywords={default:!0,enum:!0,const:!0,required:!0,maximum:!0,minimum:!0,exclusiveMaximum:!0,exclusiveMinimum:!0,multipleOf:!0,maxLength:!0,minLength:!0,pattern:!0,format:!0,maxItems:!0,minItems:!0,uniqueItems:!0,maxProperties:!0,minProperties:!0};function zc(n,e,t,i,r,s,a,o,c,u){if(i&&typeof i=="object"&&!Array.isArray(i)){e(i,r,s,a,o,c,u);for(var l in i){var h=i[l];if(Array.isArray(h)){if(l in Bi.arrayKeywords)for(var f=0;f<h.length;f++)zc(n,e,t,h[f],r+"/"+l+"/"+f,s,r,l,i,f)}else if(l in Bi.propsKeywords){if(h&&typeof h=="object")for(var p in h)zc(n,e,t,h[p],r+"/"+l+"/"+HE(p),s,r,l,i,p)}else(l in Bi.keywords||n.allKeys&&!(l in Bi.skipKeywords))&&zc(n,e,t,h,r+"/"+l,s,r,l,i)}t(i,r,s,a,o,c,u)}}function HE(n){return n.replace(/~/g,"~0").replace(/\//g,"~1")}});var _a=te(hn=>{"use strict";Object.defineProperty(hn,"__esModule",{value:!0});hn.getSchemaRefs=hn.resolveUrl=hn.normalizeId=hn._getFullPath=hn.getFullPath=hn.inlineRef=void 0;var GE=Xe(),WE=dh(),qE=Pm(),XE=new Set(["type","format","pattern","maxLength","minLength","maxProperties","minProperties","maxItems","minItems","maximum","minimum","uniqueItems","multipleOf","required","enum","const"]);function jE(n,e=!0){return typeof n=="boolean"?!0:e===!0?!fh(n):e?Im(n)<=e:!1}hn.inlineRef=jE;var YE=new Set(["$ref","$recursiveRef","$recursiveAnchor","$dynamicRef","$dynamicAnchor"]);function fh(n){for(let e in n){if(YE.has(e))return!0;let t=n[e];if(Array.isArray(t)&&t.some(fh)||typeof t=="object"&&fh(t))return!0}return!1}function Im(n){let e=0;for(let t in n){if(t==="$ref")return 1/0;if(e++,!XE.has(t)&&(typeof n[t]=="object"&&(0,GE.eachItem)(n[t],i=>e+=Im(i)),e===1/0))return 1/0}return e}function Dm(n,e="",t){t!==!1&&(e=fs(e));let i=n.parse(e);return Nm(n,i)}hn.getFullPath=Dm;function Nm(n,e){return n.serialize(e).split("#")[0]+"#"}hn._getFullPath=Nm;var ZE=/#\/?$/;function fs(n){return n?n.replace(ZE,""):""}hn.normalizeId=fs;function KE(n,e,t){return t=fs(t),n.resolve(e,t)}hn.resolveUrl=KE;var JE=/^[a-z_][-a-z0-9._]*$/i;function QE(n,e){if(typeof n=="boolean")return{};let{schemaId:t,uriResolver:i}=this.opts,r=fs(n[t]||e),s={"":r},a=Dm(i,r,!1),o={},c=new Set;return qE(n,{allKeys:!0},(h,f,p,g)=>{if(g===void 0)return;let _=a+f,m=s[g];typeof h[t]=="string"&&(m=d.call(this,h[t])),w.call(this,h.$anchor),w.call(this,h.$dynamicAnchor),s[f]=m;function d(M){let x=this.opts.uriResolver.resolve;if(M=fs(m?x(m,M):M),c.has(M))throw l(M);c.add(M);let T=this.refs[M];return typeof T=="string"&&(T=this.refs[T]),typeof T=="object"?u(h,T.schema,M):M!==fs(_)&&(M[0]==="#"?(u(h,o[M],M),o[M]=h):this.refs[M]=_),M}function w(M){if(typeof M=="string"){if(!JE.test(M))throw new Error(`invalid anchor "${M}"`);d.call(this,`#${M}`)}}}),o;function u(h,f,p){if(f!==void 0&&!WE(h,f))throw l(p)}function l(h){return new Error(`reference "${h}" resolves to more than one schema`)}}hn.getSchemaRefs=QE});var xa=te(ki=>{"use strict";Object.defineProperty(ki,"__esModule",{value:!0});ki.getData=ki.KeywordCxt=ki.validateFunctionCode=void 0;var Bm=fm(),Lm=ga(),mh=sh(),Vc=ga(),ew=xm(),va=wm(),ph=Am(),ve=Be(),De=Rn(),tw=_a(),mi=Xe(),ya=ma();function nw(n){if(Vm(n)&&($m(n),zm(n))){sw(n);return}km(n,()=>(0,Bm.topBoolOrEmptySchema)(n))}ki.validateFunctionCode=nw;function km({gen:n,validateName:e,schema:t,schemaEnv:i,opts:r},s){r.code.es5?n.func(e,(0,ve._)`${De.default.data}, ${De.default.valCxt}`,i.$async,()=>{n.code((0,ve._)`"use strict"; ${Um(t,r)}`),rw(n,r),n.code(s)}):n.func(e,(0,ve._)`${De.default.data}, ${iw(r)}`,i.$async,()=>n.code(Um(t,r)).code(s))}function iw(n){return(0,ve._)`{${De.default.instancePath}="", ${De.default.parentData}, ${De.default.parentDataProperty}, ${De.default.rootData}=${De.default.data}${n.dynamicRef?(0,ve._)`, ${De.default.dynamicAnchors}={}`:ve.nil}}={}`}function rw(n,e){n.if(De.default.valCxt,()=>{n.var(De.default.instancePath,(0,ve._)`${De.default.valCxt}.${De.default.instancePath}`),n.var(De.default.parentData,(0,ve._)`${De.default.valCxt}.${De.default.parentData}`),n.var(De.default.parentDataProperty,(0,ve._)`${De.default.valCxt}.${De.default.parentDataProperty}`),n.var(De.default.rootData,(0,ve._)`${De.default.valCxt}.${De.default.rootData}`),e.dynamicRef&&n.var(De.default.dynamicAnchors,(0,ve._)`${De.default.valCxt}.${De.default.dynamicAnchors}`)},()=>{n.var(De.default.instancePath,(0,ve._)`""`),n.var(De.default.parentData,(0,ve._)`undefined`),n.var(De.default.parentDataProperty,(0,ve._)`undefined`),n.var(De.default.rootData,De.default.data),e.dynamicRef&&n.var(De.default.dynamicAnchors,(0,ve._)`{}`)})}function sw(n){let{schema:e,opts:t,gen:i}=n;km(n,()=>{t.$comment&&e.$comment&&Gm(n),uw(n),i.let(De.default.vErrors,null),i.let(De.default.errors,0),t.unevaluated&&aw(n),Hm(n),fw(n)})}function aw(n){let{gen:e,validateName:t}=n;n.evaluated=e.const("evaluated",(0,ve._)`${t}.evaluated`),e.if((0,ve._)`${n.evaluated}.dynamicProps`,()=>e.assign((0,ve._)`${n.evaluated}.props`,(0,ve._)`undefined`)),e.if((0,ve._)`${n.evaluated}.dynamicItems`,()=>e.assign((0,ve._)`${n.evaluated}.items`,(0,ve._)`undefined`))}function Um(n,e){let t=typeof n=="object"&&n[e.schemaId];return t&&(e.code.source||e.code.process)?(0,ve._)`/*# sourceURL=${t} */`:ve.nil}function ow(n,e){if(Vm(n)&&($m(n),zm(n))){cw(n,e);return}(0,Bm.boolOrEmptySchema)(n,e)}function zm({schema:n,self:e}){if(typeof n=="boolean")return!n;for(let t in n)if(e.RULES.all[t])return!0;return!1}function Vm(n){return typeof n.schema!="boolean"}function cw(n,e){let{schema:t,gen:i,opts:r}=n;r.$comment&&t.$comment&&Gm(n),hw(n),dw(n);let s=i.const("_errs",De.default.errors);Hm(n,s),i.var(e,(0,ve._)`${s} === ${De.default.errors}`)}function $m(n){(0,mi.checkUnknownRules)(n),lw(n)}function Hm(n,e){if(n.opts.jtd)return Om(n,[],!1,e);let t=(0,Lm.getSchemaTypes)(n.schema),i=(0,Lm.coerceAndCheckDataType)(n,t);Om(n,t,!i,e)}function lw(n){let{schema:e,errSchemaPath:t,opts:i,self:r}=n;e.$ref&&i.ignoreKeywordsWithRef&&(0,mi.schemaHasRulesButRef)(e,r.RULES)&&r.logger.warn(`$ref: keywords ignored in schema at path "${t}"`)}function uw(n){let{schema:e,opts:t}=n;e.default!==void 0&&t.useDefaults&&t.strictSchema&&(0,mi.checkStrictMode)(n,"default is ignored in the schema root")}function hw(n){let e=n.schema[n.opts.schemaId];e&&(n.baseId=(0,tw.resolveUrl)(n.opts.uriResolver,n.baseId,e))}function dw(n){if(n.schema.$async&&!n.schemaEnv.$async)throw new Error("async schema in sync schema")}function Gm({gen:n,schemaEnv:e,schema:t,errSchemaPath:i,opts:r}){let s=t.$comment;if(r.$comment===!0)n.code((0,ve._)`${De.default.self}.logger.log(${s})`);else if(typeof r.$comment=="function"){let a=(0,ve.str)`${i}/$comment`,o=n.scopeValue("root",{ref:e.root});n.code((0,ve._)`${De.default.self}.opts.$comment(${s}, ${a}, ${o}.schema)`)}}function fw(n){let{gen:e,schemaEnv:t,validateName:i,ValidationError:r,opts:s}=n;t.$async?e.if((0,ve._)`${De.default.errors} === 0`,()=>e.return(De.default.data),()=>e.throw((0,ve._)`new ${r}(${De.default.vErrors})`)):(e.assign((0,ve._)`${i}.errors`,De.default.vErrors),s.unevaluated&&pw(n),e.return((0,ve._)`${De.default.errors} === 0`))}function pw({gen:n,evaluated:e,props:t,items:i}){t instanceof ve.Name&&n.assign((0,ve._)`${e}.props`,t),i instanceof ve.Name&&n.assign((0,ve._)`${e}.items`,i)}function Om(n,e,t,i){let{gen:r,schema:s,data:a,allErrors:o,opts:c,self:u}=n,{RULES:l}=u;if(s.$ref&&(c.ignoreKeywordsWithRef||!(0,mi.schemaHasRulesButRef)(s,l))){r.block(()=>qm(n,"$ref",l.all.$ref.definition));return}c.jtd||mw(n,e),r.block(()=>{for(let f of l.rules)h(f);h(l.post)});function h(f){(0,mh.shouldUseGroup)(s,f)&&(f.type?(r.if((0,Vc.checkDataType)(f.type,a,c.strictNumbers)),Fm(n,f),e.length===1&&e[0]===f.type&&t&&(r.else(),(0,Vc.reportTypeError)(n)),r.endIf()):Fm(n,f),o||r.if((0,ve._)`${De.default.errors} === ${i||0}`))}}function Fm(n,e){let{gen:t,schema:i,opts:{useDefaults:r}}=n;r&&(0,ew.assignDefaults)(n,e.type),t.block(()=>{for(let s of e.rules)(0,mh.shouldUseRule)(i,s)&&qm(n,s.keyword,s.definition,e.type)})}function mw(n,e){n.schemaEnv.meta||!n.opts.strictTypes||(gw(n,e),n.opts.allowUnionTypes||_w(n,e),yw(n,n.dataTypes))}function gw(n,e){if(e.length){if(!n.dataTypes.length){n.dataTypes=e;return}e.forEach(t=>{Wm(n.dataTypes,t)||gh(n,`type "${t}" not allowed by context "${n.dataTypes.join(",")}"`)}),xw(n,e)}}function _w(n,e){e.length>1&&!(e.length===2&&e.includes("null"))&&gh(n,"use allowUnionTypes to allow union type keyword")}function yw(n,e){let t=n.self.RULES.all;for(let i in t){let r=t[i];if(typeof r=="object"&&(0,mh.shouldUseRule)(n.schema,r)){let{type:s}=r.definition;s.length&&!s.some(a=>vw(e,a))&&gh(n,`missing type "${s.join(",")}" for keyword "${i}"`)}}}function vw(n,e){return n.includes(e)||e==="number"&&n.includes("integer")}function Wm(n,e){return n.includes(e)||e==="integer"&&n.includes("number")}function xw(n,e){let t=[];for(let i of n.dataTypes)Wm(e,i)?t.push(i):e.includes("integer")&&i==="number"&&t.push("integer");n.dataTypes=t}function gh(n,e){let t=n.schemaEnv.baseId+n.errSchemaPath;e+=` at "${t}" (strictTypes)`,(0,mi.checkStrictMode)(n,e,n.opts.strictTypes)}var $c=class{constructor(e,t,i){if((0,va.validateKeywordUsage)(e,t,i),this.gen=e.gen,this.allErrors=e.allErrors,this.keyword=i,this.data=e.data,this.schema=e.schema[i],this.$data=t.$data&&e.opts.$data&&this.schema&&this.schema.$data,this.schemaValue=(0,mi.schemaRefOrVal)(e,this.schema,i,this.$data),this.schemaType=t.schemaType,this.parentSchema=e.schema,this.params={},this.it=e,this.def=t,this.$data)this.schemaCode=e.gen.const("vSchema",Xm(this.$data,e));else if(this.schemaCode=this.schemaValue,!(0,va.validSchemaType)(this.schema,t.schemaType,t.allowUndefined))throw new Error(`${i} value must be ${JSON.stringify(t.schemaType)}`);("code"in t?t.trackErrors:t.errors!==!1)&&(this.errsCount=e.gen.const("_errs",De.default.errors))}result(e,t,i){this.failResult((0,ve.not)(e),t,i)}failResult(e,t,i){this.gen.if(e),i?i():this.error(),t?(this.gen.else(),t(),this.allErrors&&this.gen.endIf()):this.allErrors?this.gen.endIf():this.gen.else()}pass(e,t){this.failResult((0,ve.not)(e),void 0,t)}fail(e){if(e===void 0){this.error(),this.allErrors||this.gen.if(!1);return}this.gen.if(e),this.error(),this.allErrors?this.gen.endIf():this.gen.else()}fail$data(e){if(!this.$data)return this.fail(e);let{schemaCode:t}=this;this.fail((0,ve._)`${t} !== undefined && (${(0,ve.or)(this.invalid$data(),e)})`)}error(e,t,i){if(t){this.setParams(t),this._error(e,i),this.setParams({});return}this._error(e,i)}_error(e,t){(e?ya.reportExtraError:ya.reportError)(this,this.def.error,t)}$dataError(){(0,ya.reportError)(this,this.def.$dataError||ya.keyword$DataError)}reset(){if(this.errsCount===void 0)throw new Error('add "trackErrors" to keyword definition');(0,ya.resetErrorsCount)(this.gen,this.errsCount)}ok(e){this.allErrors||this.gen.if(e)}setParams(e,t){t?Object.assign(this.params,e):this.params=e}block$data(e,t,i=ve.nil){this.gen.block(()=>{this.check$data(e,i),t()})}check$data(e=ve.nil,t=ve.nil){if(!this.$data)return;let{gen:i,schemaCode:r,schemaType:s,def:a}=this;i.if((0,ve.or)((0,ve._)`${r} === undefined`,t)),e!==ve.nil&&i.assign(e,!0),(s.length||a.validateSchema)&&(i.elseIf(this.invalid$data()),this.$dataError(),e!==ve.nil&&i.assign(e,!1)),i.else()}invalid$data(){let{gen:e,schemaCode:t,schemaType:i,def:r,it:s}=this;return(0,ve.or)(a(),o());function a(){if(i.length){if(!(t instanceof ve.Name))throw new Error("ajv implementation error");let c=Array.isArray(i)?i:[i];return(0,ve._)`${(0,Vc.checkDataTypes)(c,t,s.opts.strictNumbers,Vc.DataType.Wrong)}`}return ve.nil}function o(){if(r.validateSchema){let c=e.scopeValue("validate$data",{ref:r.validateSchema});return(0,ve._)`!${c}(${t})`}return ve.nil}}subschema(e,t){let i=(0,ph.getSubschema)(this.it,e);(0,ph.extendSubschemaData)(i,this.it,e),(0,ph.extendSubschemaMode)(i,e);let r={...this.it,...i,items:void 0,props:void 0};return ow(r,t),r}mergeEvaluated(e,t){let{it:i,gen:r}=this;i.opts.unevaluated&&(i.props!==!0&&e.props!==void 0&&(i.props=mi.mergeEvaluated.props(r,e.props,i.props,t)),i.items!==!0&&e.items!==void 0&&(i.items=mi.mergeEvaluated.items(r,e.items,i.items,t)))}mergeValidEvaluated(e,t){let{it:i,gen:r}=this;if(i.opts.unevaluated&&(i.props!==!0||i.items!==!0))return r.if(t,()=>this.mergeEvaluated(e,ve.Name)),!0}};ki.KeywordCxt=$c;function qm(n,e,t,i){let r=new $c(n,t,e);"code"in t?t.code(r,i):r.$data&&t.validate?(0,va.funcKeywordCode)(r,t):"macro"in t?(0,va.macroKeywordCode)(r,t):(t.compile||t.validate)&&(0,va.funcKeywordCode)(r,t)}var Sw=/^\/(?:[^~]|~0|~1)*$/,bw=/^([0-9]+)(#|\/(?:[^~]|~0|~1)*)?$/;function Xm(n,{dataLevel:e,dataNames:t,dataPathArr:i}){let r,s;if(n==="")return De.default.rootData;if(n[0]==="/"){if(!Sw.test(n))throw new Error(`Invalid JSON-pointer: ${n}`);r=n,s=De.default.rootData}else{let u=bw.exec(n);if(!u)throw new Error(`Invalid JSON-pointer: ${n}`);let l=+u[1];if(r=u[2],r==="#"){if(l>=e)throw new Error(c("property/index",l));return i[e-l]}if(l>e)throw new Error(c("data",l));if(s=t[e-l],!r)return s}let a=s,o=r.split("/");for(let u of o)u&&(s=(0,ve._)`${s}${(0,ve.getProperty)((0,mi.unescapeJsonPointer)(u))}`,a=(0,ve._)`${a} && ${s}`);return a;function c(u,l){return`Cannot access ${u} ${l} levels up, current level is ${e}`}}ki.getData=Xm});var Hc=te(yh=>{"use strict";Object.defineProperty(yh,"__esModule",{value:!0});var _h=class extends Error{constructor(e){super("validation failed"),this.errors=e,this.ajv=this.validation=!0}};yh.default=_h});var Sa=te(Sh=>{"use strict";Object.defineProperty(Sh,"__esModule",{value:!0});var vh=_a(),xh=class extends Error{constructor(e,t,i,r){super(r||`can't resolve reference ${i} from id ${t}`),this.missingRef=(0,vh.resolveUrl)(e,t,i),this.missingSchema=(0,vh.normalizeId)((0,vh.getFullPath)(e,this.missingRef))}};Sh.default=xh});var ba=te(Pn=>{"use strict";Object.defineProperty(Pn,"__esModule",{value:!0});Pn.resolveSchema=Pn.getCompilingSchema=Pn.resolveRef=Pn.compileSchema=Pn.SchemaEnv=void 0;var zn=Be(),Mw=Hc(),gr=Rn(),Vn=_a(),jm=Xe(),Ew=xa(),ps=class{constructor(e){var t;this.refs={},this.dynamicAnchors={};let i;typeof e.schema=="object"&&(i=e.schema),this.schema=e.schema,this.schemaId=e.schemaId,this.root=e.root||this,this.baseId=(t=e.baseId)!==null&&t!==void 0?t:(0,Vn.normalizeId)(i?.[e.schemaId||"$id"]),this.schemaPath=e.schemaPath,this.localRefs=e.localRefs,this.meta=e.meta,this.$async=i?.$async,this.refs={}}};Pn.SchemaEnv=ps;function Mh(n){let e=Ym.call(this,n);if(e)return e;let t=(0,Vn.getFullPath)(this.opts.uriResolver,n.root.baseId),{es5:i,lines:r}=this.opts.code,{ownProperties:s}=this.opts,a=new zn.CodeGen(this.scope,{es5:i,lines:r,ownProperties:s}),o;n.$async&&(o=a.scopeValue("Error",{ref:Mw.default,code:(0,zn._)`require("ajv/dist/runtime/validation_error").default`}));let c=a.scopeName("validate");n.validateName=c;let u={gen:a,allErrors:this.opts.allErrors,data:gr.default.data,parentData:gr.default.parentData,parentDataProperty:gr.default.parentDataProperty,dataNames:[gr.default.data],dataPathArr:[zn.nil],dataLevel:0,dataTypes:[],definedProperties:new Set,topSchemaRef:a.scopeValue("schema",this.opts.code.source===!0?{ref:n.schema,code:(0,zn.stringify)(n.schema)}:{ref:n.schema}),validateName:c,ValidationError:o,schema:n.schema,schemaEnv:n,rootId:t,baseId:n.baseId||t,schemaPath:zn.nil,errSchemaPath:n.schemaPath||(this.opts.jtd?"":"#"),errorPath:(0,zn._)`""`,opts:this.opts,self:this},l;try{this._compilations.add(n),(0,Ew.validateFunctionCode)(u),a.optimize(this.opts.code.optimize);let h=a.toString();l=`${a.scopeRefs(gr.default.scope)}return ${h}`,this.opts.code.process&&(l=this.opts.code.process(l,n));let p=new Function(`${gr.default.self}`,`${gr.default.scope}`,l)(this,this.scope.get());if(this.scope.value(c,{ref:p}),p.errors=null,p.schema=n.schema,p.schemaEnv=n,n.$async&&(p.$async=!0),this.opts.code.source===!0&&(p.source={validateName:c,validateCode:h,scopeValues:a._values}),this.opts.unevaluated){let{props:g,items:_}=u;p.evaluated={props:g instanceof zn.Name?void 0:g,items:_ instanceof zn.Name?void 0:_,dynamicProps:g instanceof zn.Name,dynamicItems:_ instanceof zn.Name},p.source&&(p.source.evaluated=(0,zn.stringify)(p.evaluated))}return n.validate=p,n}catch(h){throw delete n.validate,delete n.validateName,l&&this.logger.error("Error compiling schema, function code:",l),h}finally{this._compilations.delete(n)}}Pn.compileSchema=Mh;function ww(n,e,t){var i;t=(0,Vn.resolveUrl)(this.opts.uriResolver,e,t);let r=n.refs[t];if(r)return r;let s=Rw.call(this,n,t);if(s===void 0){let a=(i=n.localRefs)===null||i===void 0?void 0:i[t],{schemaId:o}=this.opts;a&&(s=new ps({schema:a,schemaId:o,root:n,baseId:e}))}if(s!==void 0)return n.refs[t]=Tw.call(this,s)}Pn.resolveRef=ww;function Tw(n){return(0,Vn.inlineRef)(n.schema,this.opts.inlineRefs)?n.schema:n.validate?n:Mh.call(this,n)}function Ym(n){for(let e of this._compilations)if(Aw(e,n))return e}Pn.getCompilingSchema=Ym;function Aw(n,e){return n.schema===e.schema&&n.root===e.root&&n.baseId===e.baseId}function Rw(n,e){let t;for(;typeof(t=this.refs[e])=="string";)e=t;return t||this.schemas[e]||Gc.call(this,n,e)}function Gc(n,e){let t=this.opts.uriResolver.parse(e),i=(0,Vn._getFullPath)(this.opts.uriResolver,t),r=(0,Vn.getFullPath)(this.opts.uriResolver,n.baseId,void 0);if(Object.keys(n.schema).length>0&&i===r)return bh.call(this,t,n);let s=(0,Vn.normalizeId)(i),a=this.refs[s]||this.schemas[s];if(typeof a=="string"){let o=Gc.call(this,n,a);return typeof o?.schema!="object"?void 0:bh.call(this,t,o)}if(typeof a?.schema=="object"){if(a.validate||Mh.call(this,a),s===(0,Vn.normalizeId)(e)){let{schema:o}=a,{schemaId:c}=this.opts,u=o[c];return u&&(r=(0,Vn.resolveUrl)(this.opts.uriResolver,r,u)),new ps({schema:o,schemaId:c,root:n,baseId:r})}return bh.call(this,t,a)}}Pn.resolveSchema=Gc;var Cw=new Set(["properties","patternProperties","enum","dependencies","definitions"]);function bh(n,{baseId:e,schema:t,root:i}){var r;if(((r=n.fragment)===null||r===void 0?void 0:r[0])!=="/")return;for(let o of n.fragment.slice(1).split("/")){if(typeof t=="boolean")return;let c=t[(0,jm.unescapeFragment)(o)];if(c===void 0)return;t=c;let u=typeof t=="object"&&t[this.opts.schemaId];!Cw.has(o)&&u&&(e=(0,Vn.resolveUrl)(this.opts.uriResolver,e,u))}let s;if(typeof t!="boolean"&&t.$ref&&!(0,jm.schemaHasRulesButRef)(t,this.RULES)){let o=(0,Vn.resolveUrl)(this.opts.uriResolver,e,t.$ref);s=Gc.call(this,i,o)}let{schemaId:a}=this.opts;if(s=s||new ps({schema:t,schemaId:a,root:i,baseId:e}),s.schema!==s.root.schema)return s}});var Zm=te((T2,Pw)=>{Pw.exports={$id:"https://raw.githubusercontent.com/ajv-validator/ajv/master/lib/refs/data.json#",description:"Meta-schema for $data reference (JSON AnySchema extension proposal)",type:"object",required:["$data"],properties:{$data:{type:"string",anyOf:[{format:"relative-json-pointer"},{format:"json-pointer"}]}},additionalProperties:!1}});var Ch=te((A2,rg)=>{"use strict";var Iw=RegExp.prototype.test.bind(/^[\da-f]{8}-[\da-f]{4}-[\da-f]{4}-[\da-f]{4}-[\da-f]{12}$/iu),wh=RegExp.prototype.test.bind(/^(?:(?:25[0-5]|2[0-4]\d|1\d{2}|[1-9]\d|\d)\.){3}(?:25[0-5]|2[0-4]\d|1\d{2}|[1-9]\d|\d)$/u),Dw=RegExp.prototype.test.bind(/^\d*$/u),_r=RegExp.prototype.test.bind(/^[\da-f]{2}$/iu),Th=RegExp.prototype.test.bind(/^[\da-z\-._~]$/iu),Jm=RegExp.prototype.test.bind(/^[A-Za-z0-9\-._~!$&'()*+,;=:@/]$/u),Ah=RegExp.prototype.test.bind(/^[A-Za-z0-9\-._~!$&'()*+,;=:@/?]$/u),Nw=RegExp.prototype.test.bind(/^[A-Za-z0-9\-._~!$&'()*+,;=:]$/u),dn=new Array(256);{let n="0123456789ABCDEF";for(let e=0;e<256;e++)dn[e]="%"+n[e>>4]+n[e&15]}function jt(n){return n<2048?dn[192|n>>6]+dn[128|n&63]:n<65536?dn[224|n>>12]+dn[128|n>>6&63]+dn[128|n&63]:dn[240|n>>18]+dn[128|n>>12&63]+dn[128|n>>6&63]+dn[128|n&63]}function Lw(n){let e="",t=0,i=0;for(i=0;i<n.length;i++)if(t=n[i].charCodeAt(0),t!==48){if(!(t>=48&&t<=57||t>=65&&t<=70||t>=97&&t<=102))return"";e+=n[i];break}for(i+=1;i<n.length;i++){if(t=n[i].charCodeAt(0),!(t>=48&&t<=57||t>=65&&t<=70||t>=97&&t<=102))return"";e+=n[i]}return e}var Uw=RegExp.prototype.test.bind(/^[\dA-Fa-f]{1,4}$/),Ow=RegExp.prototype.test.bind(/^[vV][\dA-Fa-f]+\.[A-Za-z\d\-._~!$&'()*+,;=:]+$/),Fw=RegExp.prototype.test.bind(/^[A-Za-z\d\-._~]$/),Bw=RegExp.prototype.test.bind(/[^!"$&'()*+,\-.;=_`a-z{}~]/u);function kw(n){if(n.length===0)return!1;for(let e=0;e<n.length;e++)if(!Fw(n[e])){if(n[e]==="%"&&e+2<n.length&&_r(n.slice(e+1,e+3))){e+=2;continue}return!1}return!0}function Km(n){let e=-1,t=0,i=-1,r=0;for(let o=0;o<n.length;o++)n[o]==="0"?(i===-1&&(i=o),r++,r>t&&(t=r,e=i)):(i=-1,r=0);if(t<2)return n.join(":");let s=n.slice(0,e).join(":"),a=n.slice(e+t).join(":");return s+"::"+a}function zw(n){let e=n.indexOf("::");if(e!==-1&&n.indexOf("::",e+1)!==-1)return;let t=e===-1?n.split(":"):n.slice(0,e).split(":"),i=e===-1?[]:n.slice(e+2).split(":");e!==-1&&(t.length===1&&t[0]===""&&(t.length=0),i.length===1&&i[0]===""&&(i.length=0));let r=t.concat(i),s=0;for(let o=0;o<r.length;o++){let c=r[o];if(c==="")return;if(c.indexOf(".")!==-1){if(o!==r.length-1||e!==-1&&i.length===0||!wh(c))return;s+=2;continue}if(!Uw(c))return;r[o]=parseInt(c,16).toString(16),s++}if(e===-1)return s!==8?void 0:Km(r);if(s>=8)return;let a=r.slice(0,t.length);for(let o=s;o<8;o++)a.push("0");for(let o=t.length;o<r.length;o++)a.push(r[o]);return Km(a)}function Eh(n){let e=n[0]==="["&&n[n.length-1]==="]";if((n[0]==="["||n[n.length-1]==="]")&&!e)return{host:n,isIPV6:!1,error:!0};let i=e?n.slice(1,-1):n;if(e&&Ow(i))return i=i.toLowerCase(),{host:`[${i}]`,escapedHost:i,isIPV6:!1,isIPVFuture:!0};if(Vw(i,":")<2)return{host:n,isIPV6:!1,error:e};let r="",s=i.indexOf("%");if(s!==-1){let o=i.slice(s,s+3).toLowerCase()==="%25"?3:1;if(r=i.slice(s+o),!kw(r))return{host:n,isIPV6:!1,error:!0};i=i.slice(0,s)}let a=zw(i);return a===void 0?{host:n,isIPV6:!1,error:!0}:{host:a+(r?"%"+r:""),escapedHost:a+(r?"%25"+r:""),isIPV6:!0}}function Vw(n,e){let t=0;for(let i=0;i<n.length;i++)n[i]===e&&t++;return t}function Qm(n){let e=n,t=[],i=-1,r=0;for(;r=e.length;){if(r===1){if(e===".")break;if(e==="/"){t.push("/");break}else{t.push(e);break}}else if(r===2){if(e[0]==="."){if(e[1]===".")break;if(e[1]==="/"){e=e.slice(2);continue}}else if(e[0]==="/"&&(e[1]==="."||e[1]==="/")){t.push("/");break}}else if(r===3&&e==="/.."){t.length!==0&&t.pop(),t.push("/");break}if(e[0]==="."){if(e[1]==="."){if(e[2]==="/"){e=e.slice(3);continue}}else if(e[1]==="/"){e=e.slice(2);continue}}else if(e[0]==="/"&&e[1]==="."){if(e[2]==="/"){e=e.slice(2);continue}else if(e[2]==="."&&e[3]==="/"){e=e.slice(3),t.length!==0&&t.pop();continue}}if((i=e.indexOf("/",1))===-1){t.push(e);break}else t.push(e.slice(0,i)),e=e.slice(i)}return t.join("")}function $w(n){if(n===""||n[0]==="/")return Qm(n);let e=n.split("/"),t=e.length-1,i=[],r=!1;for(let s=0;s<=t;s++){let a=e[s];a==="."||a===".."?(a===".."&&(i.length!==0&&i[i.length-1]!==".."?i.pop():i.push("..")),r=s===t):i.push(a)}return r&&i[i.length-1]!==".."&&i.push(""),(i.length===0||i[0]===""||i[0].includes(":"))&&i.unshift("."),i.join("/")}var Hw={"@":"%40","/":"%2F","?":"%3F","#":"%23",":":"%3A"},Gw=/[@/?#:]/g,Ww=/[@/?#]/g;function eg(n,e){let t=e?Ww:Gw;return t.lastIndex=0,n.replace(t,i=>Hw[i])}function tg(n,e=!1){if(n.indexOf("%")===-1)return n;let t="";for(let i=0;i<n.length;i++){if(n[i]==="%"&&i+2<n.length){let r=n.slice(i+1,i+3);if(_r(r)){let s=r.toUpperCase(),a=String.fromCharCode(parseInt(s,16));e&&Th(a)?t+=a:t+="%"+s,i+=2;continue}}t+=n[i]}return t}function qw(n){let e="";for(let t=0;t<n.length;t++){let i=n[t];if(i==="%"&&t+2<n.length){let r=n.slice(t+1,t+3);if(_r(r)){let s=r.toUpperCase(),a=String.fromCharCode(parseInt(s,16));a!=="."&&Th(a)?e+=a:e+="%"+s,t+=2;continue}}if(Jm(i))e+=i;else{let r=n.charCodeAt(t);if(r<128)e+=ig(r)?i:dn[r];else if(r<55296||r>57343)e+=jt(r);else if(r<=56319&&t+1<n.length){let s=n.charCodeAt(t+1);s>=56320&&s<=57343?(e+=jt(65536+(r-55296<<10)+(s-56320)),t++):e+=jt(65533)}else e+=jt(65533)}}return e}function Xw(n,e=!1){let t="",i=e&&n[0]!=="/";for(let r=0;r<n.length;r++){let s=n[r];if(s==="%"&&r+2<n.length){let a=n.slice(r+1,r+3);if(_r(a)){t+="%"+a.toUpperCase(),r+=2;continue}}if(s==="/"&&(i=!1),Jm(s)&&(s!==":"||!i))t+=s;else{let a=n.charCodeAt(r);if(a<128)t+=dn[a];else if(a<55296||a>57343)t+=jt(a);else if(a<=56319&&r+1<n.length){let o=n.charCodeAt(r+1);o>=56320&&o<=57343?(t+=jt(65536+(a-55296<<10)+(o-56320)),r++):t+=jt(65533)}else t+=jt(65533)}}return t}function Rh(n,e){let t="";for(let i=0;i<n.length;i++){let r=n[i];if(r==="%"&&i+2<n.length){let s=n.slice(i+1,i+3);if(_r(s)){t+="%"+s.toUpperCase(),i+=2;continue}}if(e(r))t+=r;else{let s=n.charCodeAt(i);if(s<128)t+=dn[s];else if(s<55296||s>57343)t+=jt(s);else if(s<=56319&&i+1<n.length){let a=n.charCodeAt(i+1);a>=56320&&a<=57343?(t+=jt(65536+(s-55296<<10)+(a-56320)),i++):t+=jt(65533)}else t+=jt(65533)}}return t}function ng(n){return Rh(n,Nw)}function jw(n){return Rh(n,Ah)}function Yw(n){return Rh(n,Ah)}function ig(n){return n>=48&&n<=57||n>=65&&n<=90||n>=97&&n<=122||n===42||n===43||n===45||n===46||n===47||n===64||n===95}function Zw(n){let e="";for(let t=0;t<n.length;t++){let i=n[t];if(i==="%"&&t+2<n.length){let r=n.slice(t+1,t+3);if(_r(r)){let s=r.toUpperCase(),a=String.fromCharCode(parseInt(s,16));Th(a)?e+=a:e+="%"+s,t+=2;continue}}if(Ah(i))e+=i;else{let r=n.charCodeAt(t);if(r<128)e+=ig(r)?i:dn[r];else if(r<55296||r>57343)e+=jt(r);else if(r<=56319&&t+1<n.length){let s=n.charCodeAt(t+1);s>=56320&&s<=57343?(e+=jt(65536+(r-55296<<10)+(s-56320)),t++):e+=jt(65533)}else e+=jt(65533)}}return e}function Kw(n){let e="";for(let t=0;t<n.length;t++){if(n[t]==="%"&&t+2<n.length){let i=n.slice(t+1,t+3);if(_r(i)){e+="%"+i.toUpperCase(),t+=2;continue}}e+=escape(n[t])}return e}function Jw(n){let e=[];if(n.userinfo!==void 0&&(e.push(ng(n.userinfo)),e.push("@")),n.host!==void 0){let t=n.host;if(!wh(t)){let i=Eh(t);i.isIPV6!==!0&&i.isIPVFuture!==!0&&(t=tg(t,!0),i=Eh(t)),i.isIPV6===!0||i.isIPVFuture===!0?t=`[${i.escapedHost}]`:t=eg(t,!1)}e.push(t)}if(typeof n.port=="number"||typeof n.port=="string"){let t=String(n.port);if(!Dw(t))throw new TypeError("URI port is malformed.");e.push(":"),e.push(t)}return e.length?e.join(""):void 0}rg.exports={nonSimpleDomain:Bw,recomposeAuthority:Jw,reescapeHostDelimiters:eg,normalizePercentEncoding:tg,normalizePathEncoding:qw,serializePathEncoding:Xw,normalizeQueryFragmentEncoding:Zw,encodeUserinfo:ng,encodeQuery:jw,encodeFragment:Yw,escapePreservingEscapes:Kw,removeDotSegments:Qm,removeDotSegmentsRelative:$w,isIPv4:wh,isUUID:Iw,normalizeIPv6:Eh,stringArrayToHexStripped:Lw}});var lg=te((R2,cg)=>{"use strict";var{isUUID:Qw}=Ch(),eT=/^([\da-z][\d\-a-z]{0,31}):((?:[\w!$'()*+,\-./:;=@]|%[\da-f]{2})+)$/iu,tT=["http","https","ws","wss","urn","urn:uuid"];function nT(n){return tT.indexOf(n)!==-1}function Ph(n){return n.secure===!0?!0:n.secure===!1?!1:n.scheme?n.scheme.length===3&&(n.scheme[0]==="w"||n.scheme[0]==="W")&&(n.scheme[1]==="s"||n.scheme[1]==="S")&&(n.scheme[2]==="s"||n.scheme[2]==="S"):!1}function sg(n){return n.host||(n.error=n.error||"HTTP URIs must have a host."),n}function ag(n){let e=String(n.scheme).toLowerCase()==="https";return(n.port===(e?443:80)||n.port==="")&&(n.port=void 0),n.path||(n.path="/"),n}function iT(n){return n.secure=Ph(n),n.resourceName=(n.path||"/")+(n.query?"?"+n.query:""),n.path=void 0,n.query=void 0,n}function rT(n){if((n.port===(Ph(n)?443:80)||n.port==="")&&(n.port=void 0),typeof n.secure=="boolean"&&(n.scheme=n.secure?"wss":"ws",n.secure=void 0),n.resourceName){let e=n.resourceName.indexOf("?"),t=e===-1?n.resourceName:n.resourceName.slice(0,e);n.path=t&&t!=="/"?t:void 0,n.query=e===-1?void 0:n.resourceName.slice(e+1),n.resourceName=void 0}return n.fragment=void 0,n}function sT(n,e){if(!n.path)return n.error="URN can not be parsed",n;let t=n.path.match(eT);if(t&&t[0]===n.path){let i=e.scheme||n.scheme||"urn";n.nid=t[1].toLowerCase(),n.nss=t[2];let r=`${i}:${e.nid||n.nid}`,s=Ih(r);n.path=void 0,s&&(n=s.parse(n,e))}else n.error=n.error||"URN can not be parsed.";return n}function aT(n,e){if(n.nid===void 0)throw new Error("URN without nid cannot be serialized");let t=e.scheme||n.scheme||"urn",i=n.nid.toLowerCase(),r=`${t}:${e.nid||i}`,s=Ih(r);s&&(n=s.serialize(n,e));let a=n,o=n.nss;return a.path=`${i||e.nid}:${o}`,e.skipEscape=!0,a}function oT(n,e){let t=n;return t.uuid=t.nss,t.nss=void 0,!e.tolerant&&(!t.uuid||!Qw(t.uuid))&&(t.error=t.error||"UUID is not valid."),t}function cT(n){let e=n;return e.nss=(n.uuid||"").toLowerCase(),e}var og={scheme:"http",domainHost:!0,parse:sg,serialize:ag},lT={scheme:"https",domainHost:og.domainHost,parse:sg,serialize:ag},Wc={scheme:"ws",domainHost:!0,parse:iT,serialize:rT},uT={scheme:"wss",domainHost:Wc.domainHost,parse:Wc.parse,serialize:Wc.serialize},hT={scheme:"urn",parse:sT,serialize:aT,skipNormalize:!0},dT={scheme:"urn:uuid",parse:oT,serialize:cT,skipNormalize:!0},qc={http:og,https:lT,ws:Wc,wss:uT,urn:hT,"urn:uuid":dT};Object.setPrototypeOf(qc,null);function Ih(n){return n&&(qc[n]||qc[n.toLowerCase()])||void 0}cg.exports={wsIsSecure:Ph,SCHEMES:qc,isValidSchemeName:nT,getSchemeHandler:Ih}});var Eg=te((C2,Kc)=>{"use strict";var{normalizeIPv6:pg,removeDotSegments:Ea,removeDotSegmentsRelative:mg,recomposeAuthority:fT,normalizePercentEncoding:Dh,normalizePathEncoding:pT,serializePathEncoding:ug,normalizeQueryFragmentEncoding:hg,encodeQuery:mT,encodeFragment:gT,reescapeHostDelimiters:_T,isIPv4:gg,nonSimpleDomain:yT}=Ch(),{SCHEMES:_g,getSchemeHandler:Nh}=lg(),yg=/^[A-Za-z][A-Za-z0-9+.-]*$/u,vg="URI scheme is malformed.",Xc="URI host is malformed.",vT=/[\0-\x20"#/:<>?@[\\\]^`{|}\x7F]/u;function dg(n){let e=String(n);if(!yg.test(e))throw new TypeError(vg);return e}function xT(n,e){return typeof n=="string"?n=CT(n,e):typeof n=="object"&&(n=Yc(yr(n,e),e)),n}function ST(n,e,t){let i=t?Object.assign({scheme:"null"},t):{scheme:"null"},{parsed:r,malformedAuthorityOrPort:s,malformedPercentEncoding:a,malformedSchemeSpecific:o,malformedHost:c,malformedScheme:u}=jc(n,i),{parsed:l,malformedAuthorityOrPort:h,malformedPercentEncoding:f,malformedSchemeSpecific:p,malformedHost:g,malformedScheme:_}=jc(e,i);if(s||h||a||f||o||p||c||g||u||_)throw new Error(r.error||l.error||"URI is malformed.");let m=xg(r,l,i,!0),d=Nh(t&&t.scheme||m.scheme),w=m.host,M=w!==void 0&&w!==""&&(gg(w)||pg(w).isIPV6);if(bg(m,t||{},d,M))throw new Error(Xc);Sg(m,t||{},d,M);let x=w&&w.indexOf("%")!==-1&&!new RegExp("\\P{ASCII}","u").test(w);if(m.error&&!x)throw new Error(m.error);return i.skipEscape=!0,yr(m,i)}function xg(n,e,t,i){let r={};if(i||(n=Yc(yr(n,t),t),e=Yc(yr(e,t),t)),t=t||{},!t.tolerant&&e.scheme)r.scheme=e.scheme,r.userinfo=e.userinfo,r.host=e.host,r.port=e.port,r.path=Ea(e.path||""),r.query=e.query;else{if(e.userinfo!==void 0||e.host!==void 0||e.port!==void 0)r.userinfo=e.userinfo,r.host=e.host,r.port=e.port,r.path=Ea(e.path||""),r.query=e.query;else{if(!e.path)r.path=n.path,e.query!==void 0?r.query=e.query:r.query=n.query;else{if(e.path[0]==="/")r.path=Ea(e.path);else{let s=n.userinfo!==void 0||n.host!==void 0||n.port!==void 0;s&&!n.path?r.path="/"+e.path:n.path?r.path=n.path.slice(0,n.path.lastIndexOf("/")+1)+e.path:r.path=e.path,r.path=n.scheme===void 0&&!s&&r.path[0]!=="/"?mg(r.path):Ea(r.path)}r.query=e.query}r.userinfo=n.userinfo,r.host=n.host,r.port=n.port}r.scheme=n.scheme}return r.fragment=e.fragment,r}function bT(n,e,t){let i=fg(n,t),r=fg(e,t);return i!==void 0&&r!==void 0&&i===r}function yr(n,e){let t={host:n.host,scheme:n.scheme,userinfo:n.userinfo,port:n.port,path:n.path,query:n.query,nid:n.nid,nss:n.nss,uuid:n.uuid,fragment:n.fragment,reference:n.reference,resourceName:n.resourceName,secure:n.secure,error:""},i=Object.assign({},e),r=[];t.scheme&&(t.scheme=dg(t.scheme));let s=Nh(i.scheme||t.scheme);s&&s.serialize&&s.serialize(t,i);let a=t.userinfo!==void 0||t.host!==void 0||t.port!==void 0,o=(t.scheme===void 0||i.reference==="suffix")&&!a,c=!i.skipEscape&&o;t.path!==void 0&&(i.skipEscape?t.path=Dh(t.path):t.path=ug(t.path,c)),i.reference!=="suffix"&&t.scheme&&(t.scheme=dg(t.scheme),r.push(t.scheme,":"));let u=fT(t);if(u!==void 0&&(i.reference!=="suffix"&&r.push("//"),r.push(u),t.path&&t.path[0]!=="/"&&r.push("/")),t.path!==void 0){let l=t.path;!i.absolutePath&&(!s||!s.absolutePath)&&(l=o&&l[0]!=="/"?mg(l):Ea(l)),o&&(l=ug(l,!0)),u===void 0&&l[0]==="/"&&l[1]==="/"&&(l="/%2F"+l.slice(2)),r.push(l)}return t.query!==void 0&&r.push("?",mT(t.query)),t.fragment!==void 0&&r.push("#",gT(t.fragment)),r.join("")}var MT=/^(?:([^#/:?]+):)?(?:\/\/((?:([^#/?@]*)@)?(\[[^#/?\]]+\]|[^#/:?]*)(?::(\d*))?))?([^#?]*)(?:\?([^#]*))?(?:#((?:.|[\n\r])*))?/u,ET=/^(?:[^#/:?]+:)?\/\/([^/?#]*)/,wT=/^(?:[^#/:?]+:)?([/\\\t\n\r]*)/;function TT(n,e){if(e[2]!==void 0&&n.path&&n.path[0]!=="/")return'URI path must start with "/" when authority is present.';if(typeof n.port=="number"&&(n.port<0||n.port>65535))return"URI port is malformed."}function Ma(n){if(n===void 0)return!1;let e=n.indexOf("%");for(;e!==-1;){if(e+2>=n.length||!/^[\da-f]{2}$/iu.test(n.slice(e+1,e+3)))return!0;e=n.indexOf("%",e+3)}return!1}function Zc(n){return n[0]==="["&&n[n.length-1]==="]"}function AT(n){let e=n[4];return Ma(n[3])||e!==void 0&&!Zc(e)&&Ma(e)||Ma(n[6])||Ma(n[7])||Ma(n[8])}function Sg(n,e,t,i){if(bg(n,e,t,i))return n.error=n.error||Xc,!0;if(!e.unicodeSupport&&(!t||!t.unicodeSupport)&&n.host&&!Zc(n.host)&&(e.domainHost||t&&t.domainHost)&&i===!1&&yT(n.host)){let r;try{r=new URL("http://"+n.host)}catch(s){return n.error=n.error||"Host's domain name can not be converted to ASCII: "+s,!0}if(!RT(r))return n.error=n.error||Xc,!0;n.host=r.hostname}return!1}function bg(n,e,t,i){return!!(n.host&&i===!1&&!Zc(n.host)&&(e.domainHost||t&&t.domainHost)&&vT.test(n.host))}function RT(n){return n.username===""&&n.password===""&&n.port===""&&n.pathname==="/"&&n.search===""&&n.hash===""&&n.hostname!==""}function jc(n,e){let t=Object.assign({},e),i={scheme:void 0,userinfo:void 0,host:"",port:void 0,path:"",query:void 0,fragment:void 0},r=!1,s=!1,a=!1,o=!1,c=!1,u=!1,l=!1;t.reference==="suffix"&&(t.scheme?n=t.scheme+":"+n:n="//"+n);let h=n.match(ET);h!==null&&h[1].indexOf("\\")!==-1&&(i.error="URI authority must not contain a literal backslash.",r=!0);let f=n.match(wT);if(f!==null){let g=f[1],_=g.replace(/[\t\n\r]/g,"");_.length>=2&&(_.slice(0,2)!=="//"?(i.error=i.error||"URI authority must not contain a literal backslash.",r=!0):g.length!==_.length&&(i.error=i.error||"URI authority introducer must not contain whitespace.",r=!0))}let p=n.match(MT);if(p){i.scheme=p[1],i.userinfo=p[3],i.host=p[4],i.port=parseInt(p[5],10),i.path=p[6]||"",i.query=p[7],i.fragment=p[8],i.scheme!==void 0&&(yg.test(i.scheme)?i.scheme=i.scheme.toLowerCase():(i.error=i.error||vg,u=!0)),s=AT(p),s&&(i.error=i.error||"URI contains malformed percent-encoding."),isNaN(i.port)&&(i.port=p[5]);let g=TT(i,p);if(g!==void 0&&(i.error=i.error||g,r=!0),i.host)if(gg(i.host)===!1){let d=Zc(i.host),w=i.host.indexOf("[")!==-1||i.host.indexOf("]")!==-1,M=pg(i.host);l=M.isIPV6||M.isIPVFuture===!0,c=w&&(!d||M.error===!0),i.host=l?M.host:M.host.toLowerCase(),c&&(i.error=i.error||Xc,r=!0)}else l=!0;i.scheme===void 0&&i.userinfo===void 0&&i.host===void 0&&i.port===void 0&&i.query===void 0&&!i.path?i.reference="same-document":i.scheme===void 0?i.reference="relative":i.fragment===void 0?i.reference="absolute":i.reference="uri",t.reference&&t.reference!=="suffix"&&t.reference!==i.reference&&(i.error=i.error||"URI is not a "+t.reference+" reference.");let _=Nh(t.scheme||i.scheme);if(c||(o=Sg(i,t,_,l)),n.indexOf("%")!==-1&&i.host!==void 0&&!c){let m=l?i.host:Dh(i.host,!0);l||(m=Dh(m.toLowerCase())),i.host=l||t.scheme!=="null"?_T(m,l):m}(!_||_&&!_.skipNormalize)&&(i.path&&(i.path=pT(i.path)),i.query&&(i.query=hg(i.query)),i.fragment&&(i.fragment=hg(i.fragment))),_&&_.parse&&(_.parse(i,t),_===_g.urn&&i.nid===void 0&&(a=!0))}else i.error=i.error||"URI can not be parsed.";return{parsed:i,malformedAuthorityOrPort:r,malformedPercentEncoding:s,malformedSchemeSpecific:a,malformedHost:o,malformedScheme:u}}function Yc(n,e){return jc(n,e).parsed}function CT(n,e){return Mg(n,e).normalized}function Mg(n,e){let{parsed:t,malformedAuthorityOrPort:i,malformedPercentEncoding:r,malformedSchemeSpecific:s,malformedHost:a,malformedScheme:o}=jc(n,e);return{normalized:i||r||s||a||o?n:yr(t,e),malformedAuthorityOrPort:i,malformedPercentEncoding:r,malformedSchemeSpecific:s,malformedHost:a,malformedScheme:o}}function fg(n,e){if(typeof n!="string"&&typeof n!="object")return;let t;try{t=typeof n=="string"?n:yr(n,e)}catch{return}let{normalized:i,malformedAuthorityOrPort:r,malformedPercentEncoding:s,malformedSchemeSpecific:a,malformedHost:o,malformedScheme:c}=Mg(t,e);return r||s||a||o||c?void 0:i}var Lh={SCHEMES:_g,normalize:xT,resolve:ST,resolveComponent:xg,equal:bT,serialize:yr,parse:Yc};Kc.exports=Lh;Kc.exports.default=Lh;Kc.exports.fastUri=Lh});var Tg=te(Uh=>{"use strict";Object.defineProperty(Uh,"__esModule",{value:!0});var wg=Eg();wg.code='require("ajv/dist/runtime/uri").default';Uh.default=wg});var Lg=te(Ot=>{"use strict";Object.defineProperty(Ot,"__esModule",{value:!0});Ot.CodeGen=Ot.Name=Ot.nil=Ot.stringify=Ot.str=Ot._=Ot.KeywordCxt=void 0;var PT=xa();Object.defineProperty(Ot,"KeywordCxt",{enumerable:!0,get:function(){return PT.KeywordCxt}});var ms=Be();Object.defineProperty(Ot,"_",{enumerable:!0,get:function(){return ms._}});Object.defineProperty(Ot,"str",{enumerable:!0,get:function(){return ms.str}});Object.defineProperty(Ot,"stringify",{enumerable:!0,get:function(){return ms.stringify}});Object.defineProperty(Ot,"nil",{enumerable:!0,get:function(){return ms.nil}});Object.defineProperty(Ot,"Name",{enumerable:!0,get:function(){return ms.Name}});Object.defineProperty(Ot,"CodeGen",{enumerable:!0,get:function(){return ms.CodeGen}});var IT=Hc(),Ig=Sa(),DT=rh(),wa=ba(),NT=Be(),Ta=_a(),Jc=ga(),Fh=Xe(),Ag=Zm(),LT=Tg(),Dg=(n,e)=>new RegExp(n,e);Dg.code="new RegExp";var UT=["removeAdditional","useDefaults","coerceTypes"],OT=new Set(["validate","serialize","parse","wrapper","root","schema","keyword","pattern","formats","validate$data","func","obj","Error"]),FT={errorDataPath:"",format:"`validateFormats: false` can be used instead.",nullable:'"nullable" keyword is supported by default.',jsonPointers:"Deprecated jsPropertySyntax can be used instead.",extendRefs:"Deprecated ignoreKeywordsWithRef can be used instead.",missingRefs:"Pass empty schema with $id that should be ignored to ajv.addSchema.",processCode:"Use option `code: {process: (code, schemaEnv: object) => string}`",sourceCode:"Use option `code: {source: true}`",strictDefaults:"It is default now, see option `strict`.",strictKeywords:"It is default now, see option `strict`.",uniqueItems:'"uniqueItems" keyword is always validated.',unknownFormats:"Disable strict mode or pass `true` to `ajv.addFormat` (or `formats` option).",cache:"Map is used as cache, schema object as key.",serialize:"Map is used as cache, schema object as key.",ajvErrors:"It is default now."},BT={ignoreKeywordsWithRef:"",jsPropertySyntax:"",unicode:'"minLength"/"maxLength" account for unicode characters by default.'},Rg=200;function kT(n){var e,t,i,r,s,a,o,c,u,l,h,f,p,g,_,m,d,w,M,x,T,R,P,O,b;let S=n.strict,I=(e=n.code)===null||e===void 0?void 0:e.optimize,k=I===!0||I===void 0?1:I||0,V=(i=(t=n.code)===null||t===void 0?void 0:t.regExp)!==null&&i!==void 0?i:Dg,W=(r=n.uriResolver)!==null&&r!==void 0?r:LT.default;return{strictSchema:(a=(s=n.strictSchema)!==null&&s!==void 0?s:S)!==null&&a!==void 0?a:!0,strictNumbers:(c=(o=n.strictNumbers)!==null&&o!==void 0?o:S)!==null&&c!==void 0?c:!0,strictTypes:(l=(u=n.strictTypes)!==null&&u!==void 0?u:S)!==null&&l!==void 0?l:"log",strictTuples:(f=(h=n.strictTuples)!==null&&h!==void 0?h:S)!==null&&f!==void 0?f:"log",strictRequired:(g=(p=n.strictRequired)!==null&&p!==void 0?p:S)!==null&&g!==void 0?g:!1,code:n.code?{...n.code,optimize:k,regExp:V}:{optimize:k,regExp:V},loopRequired:(_=n.loopRequired)!==null&&_!==void 0?_:Rg,loopEnum:(m=n.loopEnum)!==null&&m!==void 0?m:Rg,meta:(d=n.meta)!==null&&d!==void 0?d:!0,messages:(w=n.messages)!==null&&w!==void 0?w:!0,inlineRefs:(M=n.inlineRefs)!==null&&M!==void 0?M:!0,schemaId:(x=n.schemaId)!==null&&x!==void 0?x:"$id",addUsedSchema:(T=n.addUsedSchema)!==null&&T!==void 0?T:!0,validateSchema:(R=n.validateSchema)!==null&&R!==void 0?R:!0,validateFormats:(P=n.validateFormats)!==null&&P!==void 0?P:!0,unicodeRegExp:(O=n.unicodeRegExp)!==null&&O!==void 0?O:!0,int32range:(b=n.int32range)!==null&&b!==void 0?b:!0,uriResolver:W}}var Aa=class{constructor(e={}){this.schemas={},this.refs={},this.formats={},this._compilations=new Set,this._loading={},this._cache=new Map,e=this.opts={...e,...kT(e)};let{es5:t,lines:i}=this.opts.code;this.scope=new NT.ValueScope({scope:{},prefixes:OT,es5:t,lines:i}),this.logger=WT(e.logger);let r=e.validateFormats;e.validateFormats=!1,this.RULES=(0,DT.getRules)(),Cg.call(this,FT,e,"NOT SUPPORTED"),Cg.call(this,BT,e,"DEPRECATED","warn"),this._metaOpts=HT.call(this),e.formats&&VT.call(this),this._addVocabularies(),this._addDefaultMetaSchema(),e.keywords&&$T.call(this,e.keywords),typeof e.meta=="object"&&this.addMetaSchema(e.meta),zT.call(this),e.validateFormats=r}_addVocabularies(){this.addKeyword("$async")}_addDefaultMetaSchema(){let{$data:e,meta:t,schemaId:i}=this.opts,r=Ag;i==="id"&&(r={...Ag},r.id=r.$id,delete r.$id),t&&e&&this.addMetaSchema(r,r[i],!1)}defaultMeta(){let{meta:e,schemaId:t}=this.opts;return this.opts.defaultMeta=typeof e=="object"?e[t]||e:void 0}validate(e,t){let i;if(typeof e=="string"){if(i=this.getSchema(e),!i)throw new Error(`no schema with key or ref "${e}"`)}else i=this.compile(e);let r=i(t);return"$async"in i||(this.errors=i.errors),r}compile(e,t){let i=this._addSchema(e,t);return i.validate||this._compileSchemaEnv(i)}compileAsync(e,t){if(typeof this.opts.loadSchema!="function")throw new Error("options.loadSchema should be a function");let{loadSchema:i}=this.opts;return r.call(this,e,t);async function r(l,h){await s.call(this,l.$schema);let f=this._addSchema(l,h);return f.validate||a.call(this,f)}async function s(l){l&&!this.getSchema(l)&&await r.call(this,{$ref:l},!0)}async function a(l){try{return this._compileSchemaEnv(l)}catch(h){if(!(h instanceof Ig.default))throw h;return o.call(this,h),await c.call(this,h.missingSchema),a.call(this,l)}}function o({missingSchema:l,missingRef:h}){if(this.refs[l])throw new Error(`AnySchema ${l} is loaded but ${h} cannot be resolved`)}async function c(l){let h=await u.call(this,l);this.refs[l]||await s.call(this,h.$schema),this.refs[l]||this.addSchema(h,l,t)}async function u(l){let h=this._loading[l];if(h)return h;try{return await(this._loading[l]=i(l))}finally{delete this._loading[l]}}}addSchema(e,t,i,r=this.opts.validateSchema){if(Array.isArray(e)){for(let a of e)this.addSchema(a,void 0,i,r);return this}let s;if(typeof e=="object"){let{schemaId:a}=this.opts;if(s=e[a],s!==void 0&&typeof s!="string")throw new Error(`schema ${a} must be string`)}return t=(0,Ta.normalizeId)(t||s),this._checkUnique(t),this.schemas[t]=this._addSchema(e,i,t,r,!0),this}addMetaSchema(e,t,i=this.opts.validateSchema){return this.addSchema(e,t,!0,i),this}validateSchema(e,t){if(typeof e=="boolean")return!0;let i;if(i=e.$schema,i!==void 0&&typeof i!="string")throw new Error("$schema must be a string");if(i=i||this.opts.defaultMeta||this.defaultMeta(),!i)return this.logger.warn("meta-schema not available"),this.errors=null,!0;let r=this.validate(i,e);if(!r&&t){let s="schema is invalid: "+this.errorsText();if(this.opts.validateSchema==="log")this.logger.error(s);else throw new Error(s)}return r}getSchema(e){let t;for(;typeof(t=Pg.call(this,e))=="string";)e=t;if(t===void 0){let{schemaId:i}=this.opts,r=new wa.SchemaEnv({schema:{},schemaId:i});if(t=wa.resolveSchema.call(this,r,e),!t)return;this.refs[e]=t}return t.validate||this._compileSchemaEnv(t)}removeSchema(e){if(e instanceof RegExp)return this._removeAllSchemas(this.schemas,e),this._removeAllSchemas(this.refs,e),this;switch(typeof e){case"undefined":return this._removeAllSchemas(this.schemas),this._removeAllSchemas(this.refs),this._cache.clear(),this;case"string":{let t=Pg.call(this,e);return typeof t=="object"&&this._cache.delete(t.schema),delete this.schemas[e],delete this.refs[e],this}case"object":{let t=e;this._cache.delete(t);let i=e[this.opts.schemaId];return i&&(i=(0,Ta.normalizeId)(i),delete this.schemas[i],delete this.refs[i]),this}default:throw new Error("ajv.removeSchema: invalid parameter")}}addVocabulary(e){for(let t of e)this.addKeyword(t);return this}addKeyword(e,t){let i;if(typeof e=="string")i=e,typeof t=="object"&&(this.logger.warn("these parameters are deprecated, see docs for addKeyword"),t.keyword=i);else if(typeof e=="object"&&t===void 0){if(t=e,i=t.keyword,Array.isArray(i)&&!i.length)throw new Error("addKeywords: keyword must be string or non-empty array")}else throw new Error("invalid addKeywords parameters");if(XT.call(this,i,t),!t)return(0,Fh.eachItem)(i,s=>Oh.call(this,s)),this;YT.call(this,t);let r={...t,type:(0,Jc.getJSONTypes)(t.type),schemaType:(0,Jc.getJSONTypes)(t.schemaType)};return(0,Fh.eachItem)(i,r.type.length===0?s=>Oh.call(this,s,r):s=>r.type.forEach(a=>Oh.call(this,s,r,a))),this}getKeyword(e){let t=this.RULES.all[e];return typeof t=="object"?t.definition:!!t}removeKeyword(e){let{RULES:t}=this;delete t.keywords[e],delete t.all[e];for(let i of t.rules){let r=i.rules.findIndex(s=>s.keyword===e);r>=0&&i.rules.splice(r,1)}return this}addFormat(e,t){return typeof t=="string"&&(t=new RegExp(t)),this.formats[e]=t,this}errorsText(e=this.errors,{separator:t=", ",dataVar:i="data"}={}){return!e||e.length===0?"No errors":e.map(r=>`${i}${r.instancePath} ${r.message}`).reduce((r,s)=>r+t+s)}$dataMetaSchema(e,t){let i=this.RULES.all;e=JSON.parse(JSON.stringify(e));for(let r of t){let s=r.split("/").slice(1),a=e;for(let o of s)a=a[o];for(let o in i){let c=i[o];if(typeof c!="object")continue;let{$data:u}=c.definition,l=a[o];u&&l&&(a[o]=Ng(l))}}return e}_removeAllSchemas(e,t){for(let i in e){let r=e[i];(!t||t.test(i))&&(typeof r=="string"?delete e[i]:r&&!r.meta&&(this._cache.delete(r.schema),delete e[i]))}}_addSchema(e,t,i,r=this.opts.validateSchema,s=this.opts.addUsedSchema){let a,{schemaId:o}=this.opts;if(typeof e=="object")a=e[o];else{if(this.opts.jtd)throw new Error("schema must be object");if(typeof e!="boolean")throw new Error("schema must be object or boolean")}let c=this._cache.get(e);if(c!==void 0)return c;i=(0,Ta.normalizeId)(a||i);let u=Ta.getSchemaRefs.call(this,e,i);return c=new wa.SchemaEnv({schema:e,schemaId:o,meta:t,baseId:i,localRefs:u}),this._cache.set(c.schema,c),s&&!i.startsWith("#")&&(i&&this._checkUnique(i),this.refs[i]=c),r&&this.validateSchema(e,!0),c}_checkUnique(e){if(this.schemas[e]||this.refs[e])throw new Error(`schema with key or id "${e}" already exists`)}_compileSchemaEnv(e){if(e.meta?this._compileMetaSchema(e):wa.compileSchema.call(this,e),!e.validate)throw new Error("ajv implementation error");return e.validate}_compileMetaSchema(e){let t=this.opts;this.opts=this._metaOpts;try{wa.compileSchema.call(this,e)}finally{this.opts=t}}};Aa.ValidationError=IT.default;Aa.MissingRefError=Ig.default;Ot.default=Aa;function Cg(n,e,t,i="error"){for(let r in n){let s=r;s in e&&this.logger[i](`${t}: option ${r}. ${n[s]}`)}}function Pg(n){return n=(0,Ta.normalizeId)(n),this.schemas[n]||this.refs[n]}function zT(){let n=this.opts.schemas;if(n)if(Array.isArray(n))this.addSchema(n);else for(let e in n)this.addSchema(n[e],e)}function VT(){for(let n in this.opts.formats){let e=this.opts.formats[n];e&&this.addFormat(n,e)}}function $T(n){if(Array.isArray(n)){this.addVocabulary(n);return}this.logger.warn("keywords option as map is deprecated, pass array");for(let e in n){let t=n[e];t.keyword||(t.keyword=e),this.addKeyword(t)}}function HT(){let n={...this.opts};for(let e of UT)delete n[e];return n}var GT={log(){},warn(){},error(){}};function WT(n){if(n===!1)return GT;if(n===void 0)return console;if(n.log&&n.warn&&n.error)return n;throw new Error("logger must implement log, warn and error methods")}var qT=/^[a-z_$][a-z0-9_$:-]*$/i;function XT(n,e){let{RULES:t}=this;if((0,Fh.eachItem)(n,i=>{if(t.keywords[i])throw new Error(`Keyword ${i} is already defined`);if(!qT.test(i))throw new Error(`Keyword ${i} has invalid name`)}),!!e&&e.$data&&!("code"in e||"validate"in e))throw new Error('$data keyword must have "code" or "validate" function')}function Oh(n,e,t){var i;let r=e?.post;if(t&&r)throw new Error('keyword with "post" flag cannot have "type"');let{RULES:s}=this,a=r?s.post:s.rules.find(({type:c})=>c===t);if(a||(a={type:t,rules:[]},s.rules.push(a)),s.keywords[n]=!0,!e)return;let o={keyword:n,definition:{...e,type:(0,Jc.getJSONTypes)(e.type),schemaType:(0,Jc.getJSONTypes)(e.schemaType)}};e.before?jT.call(this,a,o,e.before):a.rules.push(o),s.all[n]=o,(i=e.implements)===null||i===void 0||i.forEach(c=>this.addKeyword(c))}function jT(n,e,t){let i=n.rules.findIndex(r=>r.keyword===t);i>=0?n.rules.splice(i,0,e):(n.rules.push(e),this.logger.warn(`rule ${t} is not defined`))}function YT(n){let{metaSchema:e}=n;e!==void 0&&(n.$data&&this.opts.$data&&(e=Ng(e)),n.validateSchema=this.compile(e,!0))}var ZT={$ref:"https://raw.githubusercontent.com/ajv-validator/ajv/master/lib/refs/data.json#"};function Ng(n){return{anyOf:[n,ZT]}}});var Ug=te(Bh=>{"use strict";Object.defineProperty(Bh,"__esModule",{value:!0});var KT={keyword:"id",code(){throw new Error('NOT SUPPORTED: keyword "id", use "$id" for schema ID')}};Bh.default=KT});var tl=te(vr=>{"use strict";Object.defineProperty(vr,"__esModule",{value:!0});vr.callRef=vr.getValidate=void 0;var JT=Sa(),Og=Cn(),fn=Be(),gs=Rn(),Fg=ba(),Qc=Xe(),QT={keyword:"$ref",schemaType:"string",code(n){let{gen:e,schema:t,it:i}=n,{baseId:r,schemaEnv:s,validateName:a,opts:o,self:c}=i,{root:u}=s;if((t==="#"||t==="#/")&&r===u.baseId)return h();let l=Fg.resolveRef.call(c,u,r,t);if(l===void 0)throw new JT.default(i.opts.uriResolver,r,t);if(l instanceof Fg.SchemaEnv)return f(l);return p(l);function h(){if(s===u)return el(n,a,s,s.$async);let g=e.scopeValue("root",{ref:u});return el(n,(0,fn._)`${g}.validate`,u,u.$async)}function f(g){let _=Bg(n,g);el(n,_,g,g.$async)}function p(g){let _=e.scopeValue("schema",o.code.source===!0?{ref:g,code:(0,fn.stringify)(g)}:{ref:g}),m=e.name("valid"),d=n.subschema({schema:g,dataTypes:[],schemaPath:fn.nil,topSchemaRef:_,errSchemaPath:t},m);n.mergeEvaluated(d),n.ok(m)}}};function Bg(n,e){let{gen:t}=n;return e.validate?t.scopeValue("validate",{ref:e.validate}):(0,fn._)`${t.scopeValue("wrapper",{ref:e})}.validate`}vr.getValidate=Bg;function el(n,e,t,i){let{gen:r,it:s}=n,{allErrors:a,schemaEnv:o,opts:c}=s,u=c.passContext?gs.default.this:fn.nil;i?l():h();function l(){if(!o.$async)throw new Error("async schema referenced by sync schema");let g=r.let("valid");r.try(()=>{r.code((0,fn._)`await ${(0,Og.callValidateCode)(n,e,u)}`),p(e),a||r.assign(g,!0)},_=>{r.if((0,fn._)`!(${_} instanceof ${s.ValidationError})`,()=>r.throw(_)),f(_),a||r.assign(g,!1)}),n.ok(g)}function h(){n.result((0,Og.callValidateCode)(n,e,u),()=>p(e),()=>f(e))}function f(g){let _=(0,fn._)`${g}.errors`;r.assign(gs.default.vErrors,(0,fn._)`${gs.default.vErrors} === null ? ${_} : ${gs.default.vErrors}.concat(${_})`),r.assign(gs.default.errors,(0,fn._)`${gs.default.vErrors}.length`)}function p(g){var _;if(!s.opts.unevaluated)return;let m=(_=t?.validate)===null||_===void 0?void 0:_.evaluated;if(s.props!==!0)if(m&&!m.dynamicProps)m.props!==void 0&&(s.props=Qc.mergeEvaluated.props(r,m.props,s.props));else{let d=r.var("props",(0,fn._)`${g}.evaluated.props`);s.props=Qc.mergeEvaluated.props(r,d,s.props,fn.Name)}if(s.items!==!0)if(m&&!m.dynamicItems)m.items!==void 0&&(s.items=Qc.mergeEvaluated.items(r,m.items,s.items));else{let d=r.var("items",(0,fn._)`${g}.evaluated.items`);s.items=Qc.mergeEvaluated.items(r,d,s.items,fn.Name)}}}vr.callRef=el;vr.default=QT});var kg=te(kh=>{"use strict";Object.defineProperty(kh,"__esModule",{value:!0});var eA=Ug(),tA=tl(),nA=["$schema","$id","$defs","$vocabulary",{keyword:"$comment"},"definitions",eA.default,tA.default];kh.default=nA});var zg=te(zh=>{"use strict";Object.defineProperty(zh,"__esModule",{value:!0});var nl=Be(),zi=nl.operators,il={maximum:{okStr:"<=",ok:zi.LTE,fail:zi.GT},minimum:{okStr:">=",ok:zi.GTE,fail:zi.LT},exclusiveMaximum:{okStr:"<",ok:zi.LT,fail:zi.GTE},exclusiveMinimum:{okStr:">",ok:zi.GT,fail:zi.LTE}},iA={message:({keyword:n,schemaCode:e})=>(0,nl.str)`must be ${il[n].okStr} ${e}`,params:({keyword:n,schemaCode:e})=>(0,nl._)`{comparison: ${il[n].okStr}, limit: ${e}}`},rA={keyword:Object.keys(il),type:"number",schemaType:"number",$data:!0,error:iA,code(n){let{keyword:e,data:t,schemaCode:i}=n;n.fail$data((0,nl._)`${t} ${il[e].fail} ${i} || isNaN(${t})`)}};zh.default=rA});var Vg=te(Vh=>{"use strict";Object.defineProperty(Vh,"__esModule",{value:!0});var Ra=Be(),sA={message:({schemaCode:n})=>(0,Ra.str)`must be multiple of ${n}`,params:({schemaCode:n})=>(0,Ra._)`{multipleOf: ${n}}`},aA={keyword:"multipleOf",type:"number",schemaType:"number",$data:!0,error:sA,code(n){let{gen:e,data:t,schemaCode:i,it:r}=n,s=r.opts.multipleOfPrecision,a=e.let("res"),o=s?(0,Ra._)`Math.abs(Math.round(${a}) - ${a}) > 1e-${s}`:(0,Ra._)`${a} !== parseInt(${a})`;n.fail$data((0,Ra._)`(${i} === 0 || (${a} = ${t}/${i}, ${o}))`)}};Vh.default=aA});var Hg=te($h=>{"use strict";Object.defineProperty($h,"__esModule",{value:!0});function $g(n){let e=n.length,t=0,i=0,r;for(;i<e;)t++,r=n.charCodeAt(i++),r>=55296&&r<=56319&&i<e&&(r=n.charCodeAt(i),(r&64512)===56320&&i++);return t}$h.default=$g;$g.code='require("ajv/dist/runtime/ucs2length").default'});var Gg=te(Hh=>{"use strict";Object.defineProperty(Hh,"__esModule",{value:!0});var xr=Be(),oA=Xe(),cA=Hg(),lA={message({keyword:n,schemaCode:e}){let t=n==="maxLength"?"more":"fewer";return(0,xr.str)`must NOT have ${t} than ${e} characters`},params:({schemaCode:n})=>(0,xr._)`{limit: ${n}}`},uA={keyword:["maxLength","minLength"],type:"string",schemaType:"number",$data:!0,error:lA,code(n){let{keyword:e,data:t,schemaCode:i,it:r}=n,s=e==="maxLength"?xr.operators.GT:xr.operators.LT,a=r.opts.unicode===!1?(0,xr._)`${t}.length`:(0,xr._)`${(0,oA.useFunc)(n.gen,cA.default)}(${t})`;n.fail$data((0,xr._)`${a} ${s} ${i}`)}};Hh.default=uA});var Wg=te(Gh=>{"use strict";Object.defineProperty(Gh,"__esModule",{value:!0});var hA=Cn(),dA=Xe(),_s=Be(),fA={message:({schemaCode:n})=>(0,_s.str)`must match pattern "${n}"`,params:({schemaCode:n})=>(0,_s._)`{pattern: ${n}}`},pA={keyword:"pattern",type:"string",schemaType:"string",$data:!0,error:fA,code(n){let{gen:e,data:t,$data:i,schema:r,schemaCode:s,it:a}=n,o=a.opts.unicodeRegExp?"u":"";if(i){let{regExp:c}=a.opts.code,u=c.code==="new RegExp"?(0,_s._)`new RegExp`:(0,dA.useFunc)(e,c),l=e.let("valid");e.try(()=>e.assign(l,(0,_s._)`${u}(${s}, ${o}).test(${t})`),()=>e.assign(l,!1)),n.fail$data((0,_s._)`!${l}`)}else{let c=(0,hA.usePattern)(n,r);n.fail$data((0,_s._)`!${c}.test(${t})`)}}};Gh.default=pA});var qg=te(Wh=>{"use strict";Object.defineProperty(Wh,"__esModule",{value:!0});var Ca=Be(),mA={message({keyword:n,schemaCode:e}){let t=n==="maxProperties"?"more":"fewer";return(0,Ca.str)`must NOT have ${t} than ${e} properties`},params:({schemaCode:n})=>(0,Ca._)`{limit: ${n}}`},gA={keyword:["maxProperties","minProperties"],type:"object",schemaType:"number",$data:!0,error:mA,code(n){let{keyword:e,data:t,schemaCode:i}=n,r=e==="maxProperties"?Ca.operators.GT:Ca.operators.LT;n.fail$data((0,Ca._)`Object.keys(${t}).length ${r} ${i}`)}};Wh.default=gA});var Xg=te(qh=>{"use strict";Object.defineProperty(qh,"__esModule",{value:!0});var Pa=Cn(),Ia=Be(),_A=Xe(),yA={message:({params:{missingProperty:n}})=>(0,Ia.str)`must have required property '${n}'`,params:({params:{missingProperty:n}})=>(0,Ia._)`{missingProperty: ${n}}`},vA={keyword:"required",type:"object",schemaType:"array",$data:!0,error:yA,code(n){let{gen:e,schema:t,schemaCode:i,data:r,$data:s,it:a}=n,{opts:o}=a;if(!s&&t.length===0)return;let c=t.length>=o.loopRequired;if(a.allErrors?u():l(),o.strictRequired){let p=n.parentSchema.properties,{definedProperties:g}=n.it;for(let _ of t)if(p?.[_]===void 0&&!g.has(_)){let m=a.schemaEnv.baseId+a.errSchemaPath,d=`required property "${_}" is not defined at "${m}" (strictRequired)`;(0,_A.checkStrictMode)(a,d,a.opts.strictRequired)}}function u(){if(c||s)n.block$data(Ia.nil,h);else for(let p of t)(0,Pa.checkReportMissingProp)(n,p)}function l(){let p=e.let("missing");if(c||s){let g=e.let("valid",!0);n.block$data(g,()=>f(p,g)),n.ok(g)}else e.if((0,Pa.checkMissingProp)(n,t,p)),(0,Pa.reportMissingProp)(n,p),e.else()}function h(){e.forOf("prop",i,p=>{n.setParams({missingProperty:p}),e.if((0,Pa.noPropertyInData)(e,r,p,o.ownProperties),()=>n.error())})}function f(p,g){n.setParams({missingProperty:p}),e.forOf(p,i,()=>{e.assign(g,(0,Pa.propertyInData)(e,r,p,o.ownProperties)),e.if((0,Ia.not)(g),()=>{n.error(),e.break()})},Ia.nil)}}};qh.default=vA});var jg=te(Xh=>{"use strict";Object.defineProperty(Xh,"__esModule",{value:!0});var Da=Be(),xA={message({keyword:n,schemaCode:e}){let t=n==="maxItems"?"more":"fewer";return(0,Da.str)`must NOT have ${t} than ${e} items`},params:({schemaCode:n})=>(0,Da._)`{limit: ${n}}`},SA={keyword:["maxItems","minItems"],type:"array",schemaType:"number",$data:!0,error:xA,code(n){let{keyword:e,data:t,schemaCode:i}=n,r=e==="maxItems"?Da.operators.GT:Da.operators.LT;n.fail$data((0,Da._)`${t}.length ${r} ${i}`)}};Xh.default=SA});var rl=te(jh=>{"use strict";Object.defineProperty(jh,"__esModule",{value:!0});var Yg=dh();Yg.code='require("ajv/dist/runtime/equal").default';jh.default=Yg});var Zg=te(Zh=>{"use strict";Object.defineProperty(Zh,"__esModule",{value:!0});var Yh=ga(),Ft=Be(),bA=Xe(),MA=rl(),EA={message:({params:{i:n,j:e}})=>(0,Ft.str)`must NOT have duplicate items (items ## ${e} and ${n} are identical)`,params:({params:{i:n,j:e}})=>(0,Ft._)`{i: ${n}, j: ${e}}`},wA={keyword:"uniqueItems",type:"array",schemaType:"boolean",$data:!0,error:EA,code(n){let{gen:e,data:t,$data:i,schema:r,parentSchema:s,schemaCode:a,it:o}=n;if(!i&&!r)return;let c=e.let("valid"),u=s.items?(0,Yh.getSchemaTypes)(s.items):[];n.block$data(c,l,(0,Ft._)`${a} === false`),n.ok(c);function l(){let g=e.let("i",(0,Ft._)`${t}.length`),_=e.let("j");n.setParams({i:g,j:_}),e.assign(c,!0),e.if((0,Ft._)`${g} > 1`,()=>(h()?f:p)(g,_))}function h(){return u.length>0&&!u.some(g=>g==="object"||g==="array")}function f(g,_){let m=e.name("item"),d=(0,Yh.checkDataTypes)(u,m,o.opts.strictNumbers,Yh.DataType.Wrong),w=e.const("indices",(0,Ft._)`{}`);e.for((0,Ft._)`;${g}--;`,()=>{e.let(m,(0,Ft._)`${t}[${g}]`),e.if(d,(0,Ft._)`continue`),u.length>1&&e.if((0,Ft._)`typeof ${m} == "string"`,(0,Ft._)`${m} += "_"`),e.if((0,Ft._)`typeof ${w}[${m}] == "number"`,()=>{e.assign(_,(0,Ft._)`${w}[${m}]`),n.error(),e.assign(c,!1).break()}).code((0,Ft._)`${w}[${m}] = ${g}`)})}function p(g,_){let m=(0,bA.useFunc)(e,MA.default),d=e.name("outer");e.label(d).for((0,Ft._)`;${g}--;`,()=>e.for((0,Ft._)`${_} = ${g}; ${_}--;`,()=>e.if((0,Ft._)`${m}(${t}[${g}], ${t}[${_}])`,()=>{n.error(),e.assign(c,!1).break(d)})))}}};Zh.default=wA});var Kg=te(Jh=>{"use strict";Object.defineProperty(Jh,"__esModule",{value:!0});var Kh=Be(),TA=Xe(),AA=rl(),RA={message:"must be equal to constant",params:({schemaCode:n})=>(0,Kh._)`{allowedValue: ${n}}`},CA={keyword:"const",$data:!0,error:RA,code(n){let{gen:e,data:t,$data:i,schemaCode:r,schema:s}=n;i||s&&typeof s=="object"?n.fail$data((0,Kh._)`!${(0,TA.useFunc)(e,AA.default)}(${t}, ${r})`):n.fail((0,Kh._)`${s} !== ${t}`)}};Jh.default=CA});var Jg=te(Qh=>{"use strict";Object.defineProperty(Qh,"__esModule",{value:!0});var Na=Be(),PA=Xe(),IA=rl(),DA={message:"must be equal to one of the allowed values",params:({schemaCode:n})=>(0,Na._)`{allowedValues: ${n}}`},NA={keyword:"enum",schemaType:"array",$data:!0,error:DA,code(n){let{gen:e,data:t,$data:i,schema:r,schemaCode:s,it:a}=n;if(!i&&r.length===0)throw new Error("enum must have non-empty array");let o=r.length>=a.opts.loopEnum,c,u=()=>c??(c=(0,PA.useFunc)(e,IA.default)),l;if(o||i)l=e.let("valid"),n.block$data(l,h);else{if(!Array.isArray(r))throw new Error("ajv implementation error");let p=e.const("vSchema",s);l=(0,Na.or)(...r.map((g,_)=>f(p,_)))}n.pass(l);function h(){e.assign(l,!1),e.forOf("v",s,p=>e.if((0,Na._)`${u()}(${t}, ${p})`,()=>e.assign(l,!0).break()))}function f(p,g){let _=r[g];return typeof _=="object"&&_!==null?(0,Na._)`${u()}(${t}, ${p}[${g}])`:(0,Na._)`${t} === ${_}`}}};Qh.default=NA});var Qg=te(ed=>{"use strict";Object.defineProperty(ed,"__esModule",{value:!0});var LA=zg(),UA=Vg(),OA=Gg(),FA=Wg(),BA=qg(),kA=Xg(),zA=jg(),VA=Zg(),$A=Kg(),HA=Jg(),GA=[LA.default,UA.default,OA.default,FA.default,BA.default,kA.default,zA.default,VA.default,{keyword:"type",schemaType:["string","array"]},{keyword:"nullable",schemaType:"boolean"},$A.default,HA.default];ed.default=GA});var nd=te(La=>{"use strict";Object.defineProperty(La,"__esModule",{value:!0});La.validateAdditionalItems=void 0;var Sr=Be(),td=Xe(),WA={message:({params:{len:n}})=>(0,Sr.str)`must NOT have more than ${n} items`,params:({params:{len:n}})=>(0,Sr._)`{limit: ${n}}`},qA={keyword:"additionalItems",type:"array",schemaType:["boolean","object"],before:"uniqueItems",error:WA,code(n){let{parentSchema:e,it:t}=n,{items:i}=e;if(!Array.isArray(i)){(0,td.checkStrictMode)(t,'"additionalItems" is ignored when "items" is not an array of schemas');return}e_(n,i)}};function e_(n,e){let{gen:t,schema:i,data:r,keyword:s,it:a}=n;a.items=!0;let o=t.const("len",(0,Sr._)`${r}.length`);if(i===!1)n.setParams({len:e.length}),n.pass((0,Sr._)`${o} <= ${e.length}`);else if(typeof i=="object"&&!(0,td.alwaysValidSchema)(a,i)){let u=t.var("valid",(0,Sr._)`${o} <= ${e.length}`);t.if((0,Sr.not)(u),()=>c(u)),n.ok(u)}function c(u){t.forRange("i",e.length,o,l=>{n.subschema({keyword:s,dataProp:l,dataPropType:td.Type.Num},u),a.allErrors||t.if((0,Sr.not)(u),()=>t.break())})}}La.validateAdditionalItems=e_;La.default=qA});var id=te(Ua=>{"use strict";Object.defineProperty(Ua,"__esModule",{value:!0});Ua.validateTuple=void 0;var t_=Be(),sl=Xe(),XA=Cn(),jA={keyword:"items",type:"array",schemaType:["object","array","boolean"],before:"uniqueItems",code(n){let{schema:e,it:t}=n;if(Array.isArray(e))return n_(n,"additionalItems",e);t.items=!0,!(0,sl.alwaysValidSchema)(t,e)&&n.ok((0,XA.validateArray)(n))}};function n_(n,e,t=n.schema){let{gen:i,parentSchema:r,data:s,keyword:a,it:o}=n;l(r),o.opts.unevaluated&&t.length&&o.items!==!0&&(o.items=sl.mergeEvaluated.items(i,t.length,o.items));let c=i.name("valid"),u=i.const("len",(0,t_._)`${s}.length`);t.forEach((h,f)=>{(0,sl.alwaysValidSchema)(o,h)||(i.if((0,t_._)`${u} > ${f}`,()=>n.subschema({keyword:a,schemaProp:f,dataProp:f},c)),n.ok(c))});function l(h){let{opts:f,errSchemaPath:p}=o,g=t.length,_=g===h.minItems&&(g===h.maxItems||h[e]===!1);if(f.strictTuples&&!_){let m=`"${a}" is ${g}-tuple, but minItems or maxItems/${e} are not specified or different at path "${p}"`;(0,sl.checkStrictMode)(o,m,f.strictTuples)}}}Ua.validateTuple=n_;Ua.default=jA});var i_=te(rd=>{"use strict";Object.defineProperty(rd,"__esModule",{value:!0});var YA=id(),ZA={keyword:"prefixItems",type:"array",schemaType:["array"],before:"uniqueItems",code:n=>(0,YA.validateTuple)(n,"items")};rd.default=ZA});var s_=te(sd=>{"use strict";Object.defineProperty(sd,"__esModule",{value:!0});var r_=Be(),KA=Xe(),JA=Cn(),QA=nd(),e1={message:({params:{len:n}})=>(0,r_.str)`must NOT have more than ${n} items`,params:({params:{len:n}})=>(0,r_._)`{limit: ${n}}`},t1={keyword:"items",type:"array",schemaType:["object","boolean"],before:"uniqueItems",error:e1,code(n){let{schema:e,parentSchema:t,it:i}=n,{prefixItems:r}=t;i.items=!0,!(0,KA.alwaysValidSchema)(i,e)&&(r?(0,QA.validateAdditionalItems)(n,r):n.ok((0,JA.validateArray)(n)))}};sd.default=t1});var a_=te(ad=>{"use strict";Object.defineProperty(ad,"__esModule",{value:!0});var In=Be(),al=Xe(),n1={message:({params:{min:n,max:e}})=>e===void 0?(0,In.str)`must contain at least ${n} valid item(s)`:(0,In.str)`must contain at least ${n} and no more than ${e} valid item(s)`,params:({params:{min:n,max:e}})=>e===void 0?(0,In._)`{minContains: ${n}}`:(0,In._)`{minContains: ${n}, maxContains: ${e}}`},i1={keyword:"contains",type:"array",schemaType:["object","boolean"],before:"uniqueItems",trackErrors:!0,error:n1,code(n){let{gen:e,schema:t,parentSchema:i,data:r,it:s}=n,a,o,{minContains:c,maxContains:u}=i;s.opts.next?(a=c===void 0?1:c,o=u):a=1;let l=e.const("len",(0,In._)`${r}.length`);if(n.setParams({min:a,max:o}),o===void 0&&a===0){(0,al.checkStrictMode)(s,'"minContains" == 0 without "maxContains": "contains" keyword ignored');return}if(o!==void 0&&a>o){(0,al.checkStrictMode)(s,'"minContains" > "maxContains" is always invalid'),n.fail();return}if((0,al.alwaysValidSchema)(s,t)){let _=(0,In._)`${l} >= ${a}`;o!==void 0&&(_=(0,In._)`${_} && ${l} <= ${o}`),n.pass(_);return}s.items=!0;let h=e.name("valid");o===void 0&&a===1?p(h,()=>e.if(h,()=>e.break())):a===0?(e.let(h,!0),o!==void 0&&e.if((0,In._)`${r}.length > 0`,f)):(e.let(h,!1),f()),n.result(h,()=>n.reset());function f(){let _=e.name("_valid"),m=e.let("count",0);p(_,()=>e.if(_,()=>g(m)))}function p(_,m){e.forRange("i",0,l,d=>{n.subschema({keyword:"contains",dataProp:d,dataPropType:al.Type.Num,compositeRule:!0},_),m()})}function g(_){e.code((0,In._)`${_}++`),o===void 0?e.if((0,In._)`${_} >= ${a}`,()=>e.assign(h,!0).break()):(e.if((0,In._)`${_} > ${o}`,()=>e.assign(h,!1).break()),a===1?e.assign(h,!0):e.if((0,In._)`${_} >= ${a}`,()=>e.assign(h,!0)))}}};ad.default=i1});var ol=te(ei=>{"use strict";Object.defineProperty(ei,"__esModule",{value:!0});ei.validateSchemaDeps=ei.validatePropertyDeps=ei.error=void 0;var od=Be(),r1=Xe(),Oa=Cn();ei.error={message:({params:{property:n,depsCount:e,deps:t}})=>{let i=e===1?"property":"properties";return(0,od.str)`must have ${i} ${t} when property ${n} is present`},params:({params:{property:n,depsCount:e,deps:t,missingProperty:i}})=>(0,od._)`{property: ${n},
    missingProperty: ${i},
    depsCount: ${e},
    deps: ${t}}`};var s1={keyword:"dependencies",type:"object",schemaType:"object",error:ei.error,code(n){let[e,t]=a1(n);o_(n,e),c_(n,t)}};function a1({schema:n}){let e={},t={};for(let i in n){if(i==="__proto__")continue;let r=Array.isArray(n[i])?e:t;r[i]=n[i]}return[e,t]}function o_(n,e=n.schema){let{gen:t,data:i,it:r}=n;if(Object.keys(e).length===0)return;let s=t.let("missing");for(let a in e){let o=e[a];if(o.length===0)continue;let c=(0,Oa.propertyInData)(t,i,a,r.opts.ownProperties);n.setParams({property:a,depsCount:o.length,deps:o.join(", ")}),r.allErrors?t.if(c,()=>{for(let u of o)(0,Oa.checkReportMissingProp)(n,u)}):(t.if((0,od._)`${c} && (${(0,Oa.checkMissingProp)(n,o,s)})`),(0,Oa.reportMissingProp)(n,s),t.else())}}ei.validatePropertyDeps=o_;function c_(n,e=n.schema){let{gen:t,data:i,keyword:r,it:s}=n,a=t.name("valid");for(let o in e)(0,r1.alwaysValidSchema)(s,e[o])||(t.if((0,Oa.propertyInData)(t,i,o,s.opts.ownProperties),()=>{let c=n.subschema({keyword:r,schemaProp:o},a);n.mergeValidEvaluated(c,a)},()=>t.var(a,!0)),n.ok(a))}ei.validateSchemaDeps=c_;ei.default=s1});var u_=te(cd=>{"use strict";Object.defineProperty(cd,"__esModule",{value:!0});var l_=Be(),o1=Xe(),c1={message:"property name must be valid",params:({params:n})=>(0,l_._)`{propertyName: ${n.propertyName}}`},l1={keyword:"propertyNames",type:"object",schemaType:["object","boolean"],error:c1,code(n){let{gen:e,schema:t,data:i,it:r}=n;if((0,o1.alwaysValidSchema)(r,t))return;let s=e.name("valid");e.forIn("key",i,a=>{n.setParams({propertyName:a}),n.subschema({keyword:"propertyNames",data:a,dataTypes:["string"],propertyName:a,compositeRule:!0},s),e.if((0,l_.not)(s),()=>{n.error(!0),r.allErrors||e.break()})}),n.ok(s)}};cd.default=l1});var ud=te(ld=>{"use strict";Object.defineProperty(ld,"__esModule",{value:!0});var cl=Cn(),$n=Be(),u1=Rn(),ll=Xe(),h1={message:"must NOT have additional properties",params:({params:n})=>(0,$n._)`{additionalProperty: ${n.additionalProperty}}`},d1={keyword:"additionalProperties",type:["object"],schemaType:["boolean","object"],allowUndefined:!0,trackErrors:!0,error:h1,code(n){let{gen:e,schema:t,parentSchema:i,data:r,errsCount:s,it:a}=n;if(!s)throw new Error("ajv implementation error");let{allErrors:o,opts:c}=a;if(a.props=!0,c.removeAdditional!=="all"&&(0,ll.alwaysValidSchema)(a,t))return;let u=(0,cl.allSchemaProperties)(i.properties),l=(0,cl.allSchemaProperties)(i.patternProperties);h(),n.ok((0,$n._)`${s} === ${u1.default.errors}`);function h(){e.forIn("key",r,m=>{!u.length&&!l.length?g(m):e.if(f(m),()=>g(m))})}function f(m){let d;if(u.length>8){let w=(0,ll.schemaRefOrVal)(a,i.properties,"properties");d=(0,cl.isOwnProperty)(e,w,m)}else u.length?d=(0,$n.or)(...u.map(w=>(0,$n._)`${m} === ${w}`)):d=$n.nil;return l.length&&(d=(0,$n.or)(d,...l.map(w=>(0,$n._)`${(0,cl.usePattern)(n,w)}.test(${m})`))),(0,$n.not)(d)}function p(m){e.code((0,$n._)`delete ${r}[${m}]`)}function g(m){if(c.removeAdditional==="all"||c.removeAdditional&&t===!1){p(m);return}if(t===!1){n.setParams({additionalProperty:m}),n.error(),o||e.break();return}if(typeof t=="object"&&!(0,ll.alwaysValidSchema)(a,t)){let d=e.name("valid");c.removeAdditional==="failing"?(_(m,d,!1),e.if((0,$n.not)(d),()=>{n.reset(),p(m)})):(_(m,d),o||e.if((0,$n.not)(d),()=>e.break()))}}function _(m,d,w){let M={keyword:"additionalProperties",dataProp:m,dataPropType:ll.Type.Str};w===!1&&Object.assign(M,{compositeRule:!0,createErrors:!1,allErrors:!1}),n.subschema(M,d)}}};ld.default=d1});var f_=te(dd=>{"use strict";Object.defineProperty(dd,"__esModule",{value:!0});var f1=xa(),h_=Cn(),hd=Xe(),d_=ud(),p1={keyword:"properties",type:"object",schemaType:"object",code(n){let{gen:e,schema:t,parentSchema:i,data:r,it:s}=n;s.opts.removeAdditional==="all"&&i.additionalProperties===void 0&&d_.default.code(new f1.KeywordCxt(s,d_.default,"additionalProperties"));let a=(0,h_.allSchemaProperties)(t);for(let h of a)s.definedProperties.add(h);s.opts.unevaluated&&a.length&&s.props!==!0&&(s.props=hd.mergeEvaluated.props(e,(0,hd.toHash)(a),s.props));let o=a.filter(h=>!(0,hd.alwaysValidSchema)(s,t[h]));if(o.length===0)return;let c=e.name("valid");for(let h of o)u(h)?l(h):(e.if((0,h_.propertyInData)(e,r,h,s.opts.ownProperties)),l(h),s.allErrors||e.else().var(c,!0),e.endIf()),n.it.definedProperties.add(h),n.ok(c);function u(h){return s.opts.useDefaults&&!s.compositeRule&&t[h].default!==void 0}function l(h){n.subschema({keyword:"properties",schemaProp:h,dataProp:h},c)}}};dd.default=p1});var __=te(fd=>{"use strict";Object.defineProperty(fd,"__esModule",{value:!0});var p_=Cn(),ul=Be(),m_=Xe(),g_=Xe(),m1={keyword:"patternProperties",type:"object",schemaType:"object",code(n){let{gen:e,schema:t,data:i,parentSchema:r,it:s}=n,{opts:a}=s,o=(0,p_.allSchemaProperties)(t),c=o.filter(_=>(0,m_.alwaysValidSchema)(s,t[_]));if(o.length===0||c.length===o.length&&(!s.opts.unevaluated||s.props===!0))return;let u=a.strictSchema&&!a.allowMatchingProperties&&r.properties,l=e.name("valid");s.props!==!0&&!(s.props instanceof ul.Name)&&(s.props=(0,g_.evaluatedPropsToName)(e,s.props));let{props:h}=s;f();function f(){for(let _ of o)u&&p(_),s.allErrors?g(_):(e.var(l,!0),g(_),e.if(l))}function p(_){for(let m in u)new RegExp(_).test(m)&&(0,m_.checkStrictMode)(s,`property ${m} matches pattern ${_} (use allowMatchingProperties)`)}function g(_){e.forIn("key",i,m=>{e.if((0,ul._)`${(0,p_.usePattern)(n,_)}.test(${m})`,()=>{let d=c.includes(_);d||n.subschema({keyword:"patternProperties",schemaProp:_,dataProp:m,dataPropType:g_.Type.Str},l),s.opts.unevaluated&&h!==!0?e.assign((0,ul._)`${h}[${m}]`,!0):!d&&!s.allErrors&&e.if((0,ul.not)(l),()=>e.break())})})}}};fd.default=m1});var y_=te(pd=>{"use strict";Object.defineProperty(pd,"__esModule",{value:!0});var g1=Xe(),_1={keyword:"not",schemaType:["object","boolean"],trackErrors:!0,code(n){let{gen:e,schema:t,it:i}=n;if((0,g1.alwaysValidSchema)(i,t)){n.fail();return}let r=e.name("valid");n.subschema({keyword:"not",compositeRule:!0,createErrors:!1,allErrors:!1},r),n.failResult(r,()=>n.reset(),()=>n.error())},error:{message:"must NOT be valid"}};pd.default=_1});var v_=te(md=>{"use strict";Object.defineProperty(md,"__esModule",{value:!0});var y1=Cn(),v1={keyword:"anyOf",schemaType:"array",trackErrors:!0,code:y1.validateUnion,error:{message:"must match a schema in anyOf"}};md.default=v1});var x_=te(gd=>{"use strict";Object.defineProperty(gd,"__esModule",{value:!0});var hl=Be(),x1=Xe(),S1={message:"must match exactly one schema in oneOf",params:({params:n})=>(0,hl._)`{passingSchemas: ${n.passing}}`},b1={keyword:"oneOf",schemaType:"array",trackErrors:!0,error:S1,code(n){let{gen:e,schema:t,parentSchema:i,it:r}=n;if(!Array.isArray(t))throw new Error("ajv implementation error");if(r.opts.discriminator&&i.discriminator)return;let s=t,a=e.let("valid",!1),o=e.let("passing",null),c=e.name("_valid");n.setParams({passing:o}),e.block(u),n.result(a,()=>n.reset(),()=>n.error(!0));function u(){s.forEach((l,h)=>{let f;(0,x1.alwaysValidSchema)(r,l)?e.var(c,!0):f=n.subschema({keyword:"oneOf",schemaProp:h,compositeRule:!0},c),h>0&&e.if((0,hl._)`${c} && ${a}`).assign(a,!1).assign(o,(0,hl._)`[${o}, ${h}]`).else(),e.if(c,()=>{e.assign(a,!0),e.assign(o,h),f&&n.mergeEvaluated(f,hl.Name)})})}}};gd.default=b1});var S_=te(_d=>{"use strict";Object.defineProperty(_d,"__esModule",{value:!0});var M1=Xe(),E1={keyword:"allOf",schemaType:"array",code(n){let{gen:e,schema:t,it:i}=n;if(!Array.isArray(t))throw new Error("ajv implementation error");let r=e.name("valid");t.forEach((s,a)=>{if((0,M1.alwaysValidSchema)(i,s))return;let o=n.subschema({keyword:"allOf",schemaProp:a},r);n.ok(r),n.mergeEvaluated(o)})}};_d.default=E1});var E_=te(yd=>{"use strict";Object.defineProperty(yd,"__esModule",{value:!0});var dl=Be(),M_=Xe(),w1={message:({params:n})=>(0,dl.str)`must match "${n.ifClause}" schema`,params:({params:n})=>(0,dl._)`{failingKeyword: ${n.ifClause}}`},T1={keyword:"if",schemaType:["object","boolean"],trackErrors:!0,error:w1,code(n){let{gen:e,parentSchema:t,it:i}=n;t.then===void 0&&t.else===void 0&&(0,M_.checkStrictMode)(i,'"if" without "then" and "else" is ignored');let r=b_(i,"then"),s=b_(i,"else");if(!r&&!s)return;let a=e.let("valid",!0),o=e.name("_valid");if(c(),n.reset(),r&&s){let l=e.let("ifClause");n.setParams({ifClause:l}),e.if(o,u("then",l),u("else",l))}else r?e.if(o,u("then")):e.if((0,dl.not)(o),u("else"));n.pass(a,()=>n.error(!0));function c(){let l=n.subschema({keyword:"if",compositeRule:!0,createErrors:!1,allErrors:!1},o);n.mergeEvaluated(l)}function u(l,h){return()=>{let f=n.subschema({keyword:l},o);e.assign(a,o),n.mergeValidEvaluated(f,a),h?e.assign(h,(0,dl._)`${l}`):n.setParams({ifClause:l})}}}};function b_(n,e){let t=n.schema[e];return t!==void 0&&!(0,M_.alwaysValidSchema)(n,t)}yd.default=T1});var w_=te(vd=>{"use strict";Object.defineProperty(vd,"__esModule",{value:!0});var A1=Xe(),R1={keyword:["then","else"],schemaType:["object","boolean"],code({keyword:n,parentSchema:e,it:t}){e.if===void 0&&(0,A1.checkStrictMode)(t,`"${n}" without "if" is ignored`)}};vd.default=R1});var T_=te(xd=>{"use strict";Object.defineProperty(xd,"__esModule",{value:!0});var C1=nd(),P1=i_(),I1=id(),D1=s_(),N1=a_(),L1=ol(),U1=u_(),O1=ud(),F1=f_(),B1=__(),k1=y_(),z1=v_(),V1=x_(),$1=S_(),H1=E_(),G1=w_();function W1(n=!1){let e=[k1.default,z1.default,V1.default,$1.default,H1.default,G1.default,U1.default,O1.default,L1.default,F1.default,B1.default];return n?e.push(P1.default,D1.default):e.push(C1.default,I1.default),e.push(N1.default),e}xd.default=W1});var bd=te(Fa=>{"use strict";Object.defineProperty(Fa,"__esModule",{value:!0});Fa.dynamicAnchor=void 0;var Sd=Be(),q1=Rn(),A_=ba(),X1=tl(),j1={keyword:"$dynamicAnchor",schemaType:"string",code:n=>R_(n,n.schema)};function R_(n,e){let{gen:t,it:i}=n;i.schemaEnv.root.dynamicAnchors[e]=!0;let r=(0,Sd._)`${q1.default.dynamicAnchors}${(0,Sd.getProperty)(e)}`,s=i.errSchemaPath==="#"?i.validateName:Y1(n);t.if((0,Sd._)`!${r}`,()=>t.assign(r,s))}Fa.dynamicAnchor=R_;function Y1(n){let{schemaEnv:e,schema:t,self:i}=n.it,{root:r,baseId:s,localRefs:a,meta:o}=e.root,{schemaId:c}=i.opts,u=new A_.SchemaEnv({schema:t,schemaId:c,root:r,baseId:s,localRefs:a,meta:o});return A_.compileSchema.call(i,u),(0,X1.getValidate)(n,u)}Fa.default=j1});var Md=te(Ba=>{"use strict";Object.defineProperty(Ba,"__esModule",{value:!0});Ba.dynamicRef=void 0;var C_=Be(),Z1=Rn(),P_=tl(),K1={keyword:"$dynamicRef",schemaType:"string",code:n=>I_(n,n.schema)};function I_(n,e){let{gen:t,keyword:i,it:r}=n;if(e[0]!=="#")throw new Error(`"${i}" only supports hash fragment reference`);let s=e.slice(1);if(r.allErrors)a();else{let c=t.let("valid",!1);a(c),n.ok(c)}function a(c){if(r.schemaEnv.root.dynamicAnchors[s]){let u=t.let("_v",(0,C_._)`${Z1.default.dynamicAnchors}${(0,C_.getProperty)(s)}`);t.if(u,o(u,c),o(r.validateName,c))}else o(r.validateName,c)()}function o(c,u){return u?()=>t.block(()=>{(0,P_.callRef)(n,c),t.let(u,!0)}):()=>(0,P_.callRef)(n,c)}}Ba.dynamicRef=I_;Ba.default=K1});var D_=te(Ed=>{"use strict";Object.defineProperty(Ed,"__esModule",{value:!0});var J1=bd(),Q1=Xe(),eR={keyword:"$recursiveAnchor",schemaType:"boolean",code(n){n.schema?(0,J1.dynamicAnchor)(n,""):(0,Q1.checkStrictMode)(n.it,"$recursiveAnchor: false is ignored")}};Ed.default=eR});var N_=te(wd=>{"use strict";Object.defineProperty(wd,"__esModule",{value:!0});var tR=Md(),nR={keyword:"$recursiveRef",schemaType:"string",code:n=>(0,tR.dynamicRef)(n,n.schema)};wd.default=nR});var L_=te(Td=>{"use strict";Object.defineProperty(Td,"__esModule",{value:!0});var iR=bd(),rR=Md(),sR=D_(),aR=N_(),oR=[iR.default,rR.default,sR.default,aR.default];Td.default=oR});var O_=te(Ad=>{"use strict";Object.defineProperty(Ad,"__esModule",{value:!0});var U_=ol(),cR={keyword:"dependentRequired",type:"object",schemaType:"object",error:U_.error,code:n=>(0,U_.validatePropertyDeps)(n)};Ad.default=cR});var F_=te(Rd=>{"use strict";Object.defineProperty(Rd,"__esModule",{value:!0});var lR=ol(),uR={keyword:"dependentSchemas",type:"object",schemaType:"object",code:n=>(0,lR.validateSchemaDeps)(n)};Rd.default=uR});var B_=te(Cd=>{"use strict";Object.defineProperty(Cd,"__esModule",{value:!0});var hR=Xe(),dR={keyword:["maxContains","minContains"],type:"array",schemaType:"number",code({keyword:n,parentSchema:e,it:t}){e.contains===void 0&&(0,hR.checkStrictMode)(t,`"${n}" without "contains" is ignored`)}};Cd.default=dR});var k_=te(Pd=>{"use strict";Object.defineProperty(Pd,"__esModule",{value:!0});var fR=O_(),pR=F_(),mR=B_(),gR=[fR.default,pR.default,mR.default];Pd.default=gR});var V_=te(Id=>{"use strict";Object.defineProperty(Id,"__esModule",{value:!0});var Vi=Be(),z_=Xe(),_R=Rn(),yR={message:"must NOT have unevaluated properties",params:({params:n})=>(0,Vi._)`{unevaluatedProperty: ${n.unevaluatedProperty}}`},vR={keyword:"unevaluatedProperties",type:"object",schemaType:["boolean","object"],trackErrors:!0,error:yR,code(n){let{gen:e,schema:t,data:i,errsCount:r,it:s}=n;if(!r)throw new Error("ajv implementation error");let{allErrors:a,props:o}=s;o instanceof Vi.Name?e.if((0,Vi._)`${o} !== true`,()=>e.forIn("key",i,h=>e.if(u(o,h),()=>c(h)))):o!==!0&&e.forIn("key",i,h=>o===void 0?c(h):e.if(l(o,h),()=>c(h))),s.props=!0,n.ok((0,Vi._)`${r} === ${_R.default.errors}`);function c(h){if(t===!1){n.setParams({unevaluatedProperty:h}),n.error(),a||e.break();return}if(!(0,z_.alwaysValidSchema)(s,t)){let f=e.name("valid");n.subschema({keyword:"unevaluatedProperties",dataProp:h,dataPropType:z_.Type.Str},f),a||e.if((0,Vi.not)(f),()=>e.break())}}function u(h,f){return(0,Vi._)`!${h} || !${h}[${f}]`}function l(h,f){let p=[];for(let g in h)h[g]===!0&&p.push((0,Vi._)`${f} !== ${g}`);return(0,Vi.and)(...p)}}};Id.default=vR});var H_=te(Dd=>{"use strict";Object.defineProperty(Dd,"__esModule",{value:!0});var br=Be(),$_=Xe(),xR={message:({params:{len:n}})=>(0,br.str)`must NOT have more than ${n} items`,params:({params:{len:n}})=>(0,br._)`{limit: ${n}}`},SR={keyword:"unevaluatedItems",type:"array",schemaType:["boolean","object"],error:xR,code(n){let{gen:e,schema:t,data:i,it:r}=n,s=r.items||0;if(s===!0)return;let a=e.const("len",(0,br._)`${i}.length`);if(t===!1)n.setParams({len:s}),n.fail((0,br._)`${a} > ${s}`);else if(typeof t=="object"&&!(0,$_.alwaysValidSchema)(r,t)){let c=e.var("valid",(0,br._)`${a} <= ${s}`);e.if((0,br.not)(c),()=>o(c,s)),n.ok(c)}r.items=!0;function o(c,u){e.forRange("i",u,a,l=>{n.subschema({keyword:"unevaluatedItems",dataProp:l,dataPropType:$_.Type.Num},c),r.allErrors||e.if((0,br.not)(c),()=>e.break())})}}};Dd.default=SR});var G_=te(Nd=>{"use strict";Object.defineProperty(Nd,"__esModule",{value:!0});var bR=V_(),MR=H_(),ER=[bR.default,MR.default];Nd.default=ER});var W_=te(Ld=>{"use strict";Object.defineProperty(Ld,"__esModule",{value:!0});var Rt=Be(),wR={message:({schemaCode:n})=>(0,Rt.str)`must match format "${n}"`,params:({schemaCode:n})=>(0,Rt._)`{format: ${n}}`},TR={keyword:"format",type:["number","string"],schemaType:"string",$data:!0,error:wR,code(n,e){let{gen:t,data:i,$data:r,schema:s,schemaCode:a,it:o}=n,{opts:c,errSchemaPath:u,schemaEnv:l,self:h}=o;if(!c.validateFormats)return;r?f():p();function f(){let g=t.scopeValue("formats",{ref:h.formats,code:c.code.formats}),_=t.const("fDef",(0,Rt._)`${g}[${a}]`),m=t.let("fType"),d=t.let("format");t.if((0,Rt._)`typeof ${_} == "object" && !(${_} instanceof RegExp)`,()=>t.assign(m,(0,Rt._)`${_}.type || "string"`).assign(d,(0,Rt._)`${_}.validate`),()=>t.assign(m,(0,Rt._)`"string"`).assign(d,_)),n.fail$data((0,Rt.or)(w(),M()));function w(){return c.strictSchema===!1?Rt.nil:(0,Rt._)`${a} && !${d}`}function M(){let x=l.$async?(0,Rt._)`(${_}.async ? await ${d}(${i}) : ${d}(${i}))`:(0,Rt._)`${d}(${i})`,T=(0,Rt._)`(typeof ${d} == "function" ? ${x} : ${d}.test(${i}))`;return(0,Rt._)`${d} && ${d} !== true && ${m} === ${e} && !${T}`}}function p(){let g=h.formats[s];if(!g){w();return}if(g===!0)return;let[_,m,d]=M(g);_===e&&n.pass(x());function w(){if(c.strictSchema===!1){h.logger.warn(T());return}throw new Error(T());function T(){return`unknown format "${s}" ignored in schema at path "${u}"`}}function M(T){let R=T instanceof RegExp?(0,Rt.regexpCode)(T):c.code.formats?(0,Rt._)`${c.code.formats}${(0,Rt.getProperty)(s)}`:void 0,P=t.scopeValue("formats",{key:s,ref:T,code:R});return typeof T=="object"&&!(T instanceof RegExp)?[T.type||"string",T.validate,(0,Rt._)`${P}.validate`]:["string",T,P]}function x(){if(typeof g=="object"&&!(g instanceof RegExp)&&g.async){if(!l.$async)throw new Error("async format in sync schema");return(0,Rt._)`await ${d}(${i})`}return typeof m=="function"?(0,Rt._)`${d}(${i})`:(0,Rt._)`${d}.test(${i})`}}}};Ld.default=TR});var q_=te(Ud=>{"use strict";Object.defineProperty(Ud,"__esModule",{value:!0});var AR=W_(),RR=[AR.default];Ud.default=RR});var X_=te(ys=>{"use strict";Object.defineProperty(ys,"__esModule",{value:!0});ys.contentVocabulary=ys.metadataVocabulary=void 0;ys.metadataVocabulary=["title","description","default","deprecated","readOnly","writeOnly","examples"];ys.contentVocabulary=["contentMediaType","contentEncoding","contentSchema"]});var Y_=te(Od=>{"use strict";Object.defineProperty(Od,"__esModule",{value:!0});var CR=kg(),PR=Qg(),IR=T_(),DR=L_(),NR=k_(),LR=G_(),UR=q_(),j_=X_(),OR=[DR.default,CR.default,PR.default,(0,IR.default)(!0),UR.default,j_.metadataVocabulary,j_.contentVocabulary,NR.default,LR.default];Od.default=OR});var K_=te(fl=>{"use strict";Object.defineProperty(fl,"__esModule",{value:!0});fl.DiscrError=void 0;var Z_;(function(n){n.Tag="tag",n.Mapping="mapping"})(Z_||(fl.DiscrError=Z_={}))});var Q_=te(Bd=>{"use strict";Object.defineProperty(Bd,"__esModule",{value:!0});var vs=Be(),Fd=K_(),J_=ba(),FR=Sa(),BR=Xe(),kR={message:({params:{discrError:n,tagName:e}})=>n===Fd.DiscrError.Tag?`tag "${e}" must be string`:`value of tag "${e}" must be in oneOf`,params:({params:{discrError:n,tag:e,tagName:t}})=>(0,vs._)`{error: ${n}, tag: ${t}, tagValue: ${e}}`},zR={keyword:"discriminator",type:"object",schemaType:"object",error:kR,code(n){let{gen:e,data:t,schema:i,parentSchema:r,it:s}=n,{oneOf:a}=r;if(!s.opts.discriminator)throw new Error("discriminator: requires discriminator option");let o=i.propertyName;if(typeof o!="string")throw new Error("discriminator: requires propertyName");if(i.mapping)throw new Error("discriminator: mapping is not supported");if(!a)throw new Error("discriminator: requires oneOf keyword");let c=e.let("valid",!1),u=e.const("tag",(0,vs._)`${t}${(0,vs.getProperty)(o)}`);e.if((0,vs._)`typeof ${u} == "string"`,()=>l(),()=>n.error(!1,{discrError:Fd.DiscrError.Tag,tag:u,tagName:o})),n.ok(c);function l(){let p=f();e.if(!1);for(let g in p)e.elseIf((0,vs._)`${u} === ${g}`),e.assign(c,h(p[g]));e.else(),n.error(!1,{discrError:Fd.DiscrError.Mapping,tag:u,tagName:o}),e.endIf()}function h(p){let g=e.name("valid"),_=n.subschema({keyword:"oneOf",schemaProp:p},g);return n.mergeEvaluated(_,vs.Name),g}function f(){var p;let g={},_=d(r),m=!0;for(let x=0;x<a.length;x++){let T=a[x];if(T?.$ref&&!(0,BR.schemaHasRulesButRef)(T,s.self.RULES)){let P=T.$ref;if(T=J_.resolveRef.call(s.self,s.schemaEnv.root,s.baseId,P),T instanceof J_.SchemaEnv&&(T=T.schema),T===void 0)throw new FR.default(s.opts.uriResolver,s.baseId,P)}let R=(p=T?.properties)===null||p===void 0?void 0:p[o];if(typeof R!="object")throw new Error(`discriminator: oneOf subschemas (or referenced schemas) must have "properties/${o}"`);m=m&&(_||d(T)),w(R,x)}if(!m)throw new Error(`discriminator: "${o}" must be required`);return g;function d({required:x}){return Array.isArray(x)&&x.includes(o)}function w(x,T){if(x.const)M(x.const,T);else if(x.enum)for(let R of x.enum)M(R,T);else throw new Error(`discriminator: "properties/${o}" must have "const" or "enum"`)}function M(x,T){if(typeof x!="string"||x in g)throw new Error(`discriminator: "${o}" values must be unique strings`);g[x]=T}}}};Bd.default=zR});var e0=te((CN,VR)=>{VR.exports={$schema:"https://json-schema.org/draft/2020-12/schema",$id:"https://json-schema.org/draft/2020-12/schema",$vocabulary:{"https://json-schema.org/draft/2020-12/vocab/core":!0,"https://json-schema.org/draft/2020-12/vocab/applicator":!0,"https://json-schema.org/draft/2020-12/vocab/unevaluated":!0,"https://json-schema.org/draft/2020-12/vocab/validation":!0,"https://json-schema.org/draft/2020-12/vocab/meta-data":!0,"https://json-schema.org/draft/2020-12/vocab/format-annotation":!0,"https://json-schema.org/draft/2020-12/vocab/content":!0},$dynamicAnchor:"meta",title:"Core and Validation specifications meta-schema",allOf:[{$ref:"meta/core"},{$ref:"meta/applicator"},{$ref:"meta/unevaluated"},{$ref:"meta/validation"},{$ref:"meta/meta-data"},{$ref:"meta/format-annotation"},{$ref:"meta/content"}],type:["object","boolean"],$comment:"This meta-schema also defines keywords that have appeared in previous drafts in order to prevent incompatible extensions as they remain in common use.",properties:{definitions:{$comment:'"definitions" has been replaced by "$defs".',type:"object",additionalProperties:{$dynamicRef:"#meta"},deprecated:!0,default:{}},dependencies:{$comment:'"dependencies" has been split and replaced by "dependentSchemas" and "dependentRequired" in order to serve their differing semantics.',type:"object",additionalProperties:{anyOf:[{$dynamicRef:"#meta"},{$ref:"meta/validation#/$defs/stringArray"}]},deprecated:!0,default:{}},$recursiveAnchor:{$comment:'"$recursiveAnchor" has been replaced by "$dynamicAnchor".',$ref:"meta/core#/$defs/anchorString",deprecated:!0},$recursiveRef:{$comment:'"$recursiveRef" has been replaced by "$dynamicRef".',$ref:"meta/core#/$defs/uriReferenceString",deprecated:!0}}}});var t0=te((PN,$R)=>{$R.exports={$schema:"https://json-schema.org/draft/2020-12/schema",$id:"https://json-schema.org/draft/2020-12/meta/applicator",$vocabulary:{"https://json-schema.org/draft/2020-12/vocab/applicator":!0},$dynamicAnchor:"meta",title:"Applicator vocabulary meta-schema",type:["object","boolean"],properties:{prefixItems:{$ref:"#/$defs/schemaArray"},items:{$dynamicRef:"#meta"},contains:{$dynamicRef:"#meta"},additionalProperties:{$dynamicRef:"#meta"},properties:{type:"object",additionalProperties:{$dynamicRef:"#meta"},default:{}},patternProperties:{type:"object",additionalProperties:{$dynamicRef:"#meta"},propertyNames:{format:"regex"},default:{}},dependentSchemas:{type:"object",additionalProperties:{$dynamicRef:"#meta"},default:{}},propertyNames:{$dynamicRef:"#meta"},if:{$dynamicRef:"#meta"},then:{$dynamicRef:"#meta"},else:{$dynamicRef:"#meta"},allOf:{$ref:"#/$defs/schemaArray"},anyOf:{$ref:"#/$defs/schemaArray"},oneOf:{$ref:"#/$defs/schemaArray"},not:{$dynamicRef:"#meta"}},$defs:{schemaArray:{type:"array",minItems:1,items:{$dynamicRef:"#meta"}}}}});var n0=te((IN,HR)=>{HR.exports={$schema:"https://json-schema.org/draft/2020-12/schema",$id:"https://json-schema.org/draft/2020-12/meta/unevaluated",$vocabulary:{"https://json-schema.org/draft/2020-12/vocab/unevaluated":!0},$dynamicAnchor:"meta",title:"Unevaluated applicator vocabulary meta-schema",type:["object","boolean"],properties:{unevaluatedItems:{$dynamicRef:"#meta"},unevaluatedProperties:{$dynamicRef:"#meta"}}}});var i0=te((DN,GR)=>{GR.exports={$schema:"https://json-schema.org/draft/2020-12/schema",$id:"https://json-schema.org/draft/2020-12/meta/content",$vocabulary:{"https://json-schema.org/draft/2020-12/vocab/content":!0},$dynamicAnchor:"meta",title:"Content vocabulary meta-schema",type:["object","boolean"],properties:{contentEncoding:{type:"string"},contentMediaType:{type:"string"},contentSchema:{$dynamicRef:"#meta"}}}});var r0=te((NN,WR)=>{WR.exports={$schema:"https://json-schema.org/draft/2020-12/schema",$id:"https://json-schema.org/draft/2020-12/meta/core",$vocabulary:{"https://json-schema.org/draft/2020-12/vocab/core":!0},$dynamicAnchor:"meta",title:"Core vocabulary meta-schema",type:["object","boolean"],properties:{$id:{$ref:"#/$defs/uriReferenceString",$comment:"Non-empty fragments not allowed.",pattern:"^[^#]*#?$"},$schema:{$ref:"#/$defs/uriString"},$ref:{$ref:"#/$defs/uriReferenceString"},$anchor:{$ref:"#/$defs/anchorString"},$dynamicRef:{$ref:"#/$defs/uriReferenceString"},$dynamicAnchor:{$ref:"#/$defs/anchorString"},$vocabulary:{type:"object",propertyNames:{$ref:"#/$defs/uriString"},additionalProperties:{type:"boolean"}},$comment:{type:"string"},$defs:{type:"object",additionalProperties:{$dynamicRef:"#meta"}}},$defs:{anchorString:{type:"string",pattern:"^[A-Za-z_][-A-Za-z0-9._]*$"},uriString:{type:"string",format:"uri"},uriReferenceString:{type:"string",format:"uri-reference"}}}});var s0=te((LN,qR)=>{qR.exports={$schema:"https://json-schema.org/draft/2020-12/schema",$id:"https://json-schema.org/draft/2020-12/meta/format-annotation",$vocabulary:{"https://json-schema.org/draft/2020-12/vocab/format-annotation":!0},$dynamicAnchor:"meta",title:"Format vocabulary meta-schema for annotation results",type:["object","boolean"],properties:{format:{type:"string"}}}});var a0=te((UN,XR)=>{XR.exports={$schema:"https://json-schema.org/draft/2020-12/schema",$id:"https://json-schema.org/draft/2020-12/meta/meta-data",$vocabulary:{"https://json-schema.org/draft/2020-12/vocab/meta-data":!0},$dynamicAnchor:"meta",title:"Meta-data vocabulary meta-schema",type:["object","boolean"],properties:{title:{type:"string"},description:{type:"string"},default:!0,deprecated:{type:"boolean",default:!1},readOnly:{type:"boolean",default:!1},writeOnly:{type:"boolean",default:!1},examples:{type:"array",items:!0}}}});var o0=te((ON,jR)=>{jR.exports={$schema:"https://json-schema.org/draft/2020-12/schema",$id:"https://json-schema.org/draft/2020-12/meta/validation",$vocabulary:{"https://json-schema.org/draft/2020-12/vocab/validation":!0},$dynamicAnchor:"meta",title:"Validation vocabulary meta-schema",type:["object","boolean"],properties:{type:{anyOf:[{$ref:"#/$defs/simpleTypes"},{type:"array",items:{$ref:"#/$defs/simpleTypes"},minItems:1,uniqueItems:!0}]},const:!0,enum:{type:"array",items:!0},multipleOf:{type:"number",exclusiveMinimum:0},maximum:{type:"number"},exclusiveMaximum:{type:"number"},minimum:{type:"number"},exclusiveMinimum:{type:"number"},maxLength:{$ref:"#/$defs/nonNegativeInteger"},minLength:{$ref:"#/$defs/nonNegativeIntegerDefault0"},pattern:{type:"string",format:"regex"},maxItems:{$ref:"#/$defs/nonNegativeInteger"},minItems:{$ref:"#/$defs/nonNegativeIntegerDefault0"},uniqueItems:{type:"boolean",default:!1},maxContains:{$ref:"#/$defs/nonNegativeInteger"},minContains:{$ref:"#/$defs/nonNegativeInteger",default:1},maxProperties:{$ref:"#/$defs/nonNegativeInteger"},minProperties:{$ref:"#/$defs/nonNegativeIntegerDefault0"},required:{$ref:"#/$defs/stringArray"},dependentRequired:{type:"object",additionalProperties:{$ref:"#/$defs/stringArray"}}},$defs:{nonNegativeInteger:{type:"integer",minimum:0},nonNegativeIntegerDefault0:{$ref:"#/$defs/nonNegativeInteger",default:0},simpleTypes:{enum:["array","boolean","integer","null","number","object","string"]},stringArray:{type:"array",items:{type:"string"},uniqueItems:!0,default:[]}}}});var c0=te(kd=>{"use strict";Object.defineProperty(kd,"__esModule",{value:!0});var YR=e0(),ZR=t0(),KR=n0(),JR=i0(),QR=r0(),eC=s0(),tC=a0(),nC=o0(),iC=["/properties"];function rC(n){return[YR,ZR,KR,JR,QR,e(this,eC),tC,e(this,nC)].forEach(t=>this.addMetaSchema(t,void 0,!1)),this;function e(t,i){return n?t.$dataMetaSchema(i,iC):i}}kd.default=rC});var l0=te((vt,Vd)=>{"use strict";Object.defineProperty(vt,"__esModule",{value:!0});vt.MissingRefError=vt.ValidationError=vt.CodeGen=vt.Name=vt.nil=vt.stringify=vt.str=vt._=vt.KeywordCxt=vt.Ajv2020=void 0;var sC=Lg(),aC=Y_(),oC=Q_(),cC=c0(),zd="https://json-schema.org/draft/2020-12/schema",xs=class extends sC.default{constructor(e={}){super({...e,dynamicRef:!0,next:!0,unevaluated:!0})}_addVocabularies(){super._addVocabularies(),aC.default.forEach(e=>this.addVocabulary(e)),this.opts.discriminator&&this.addKeyword(oC.default)}_addDefaultMetaSchema(){super._addDefaultMetaSchema();let{$data:e,meta:t}=this.opts;t&&(cC.default.call(this,e),this.refs["http://json-schema.org/schema"]=zd)}defaultMeta(){return this.opts.defaultMeta=super.defaultMeta()||(this.getSchema(zd)?zd:void 0)}};vt.Ajv2020=xs;Vd.exports=vt=xs;Vd.exports.Ajv2020=xs;Object.defineProperty(vt,"__esModule",{value:!0});vt.default=xs;var lC=xa();Object.defineProperty(vt,"KeywordCxt",{enumerable:!0,get:function(){return lC.KeywordCxt}});var Ss=Be();Object.defineProperty(vt,"_",{enumerable:!0,get:function(){return Ss._}});Object.defineProperty(vt,"str",{enumerable:!0,get:function(){return Ss.str}});Object.defineProperty(vt,"stringify",{enumerable:!0,get:function(){return Ss.stringify}});Object.defineProperty(vt,"nil",{enumerable:!0,get:function(){return Ss.nil}});Object.defineProperty(vt,"Name",{enumerable:!0,get:function(){return Ss.Name}});Object.defineProperty(vt,"CodeGen",{enumerable:!0,get:function(){return Ss.CodeGen}});var uC=Hc();Object.defineProperty(vt,"ValidationError",{enumerable:!0,get:function(){return uC.default}});var hC=Sa();Object.defineProperty(vt,"MissingRefError",{enumerable:!0,get:function(){return hC.default}})});var Pi={LEFT:0,MIDDLE:1,RIGHT:2,ROTATE:0,DOLLY:1,PAN:2},Ii={ROTATE:0,PAN:1,DOLLY_PAN:2,DOLLY_ROTATE:3},If=0,eu=1,Df=2;var tu=1,Nf=2,jn=3,ci=0,Jt=1,Yn=2,ui=0,Ki=1,nu=2,iu=3,ru=4,Lf=5,wi=100,Uf=101,Of=102,Ff=103,Bf=104,kf=200,zf=201,Vf=202,$f=203,lo=204,uo=205,Hf=206,Gf=207,Wf=208,qf=209,Xf=210,jf=211,Yf=212,Zf=213,Kf=214,Lo=0,Uo=1,Oo=2,Ji=3,Fo=4,Bo=5,ko=6,zo=7,su=0,Jf=1,Qf=2,hi=0,ep=1,tp=2,np=3,ip=4,rp=5,sp=6,ap=7;var au=300,ir=301,rr=302,Vo=303,$o=304,Qs=306,ho=1e3,Ei=1001,fo=1002,sn=1003,op=1004;var ea=1005;var On=1006,Ho=1007;var Di=1008;var Zn=1009,ou=1010,cu=1011,Jr=1012,Go=1013,Ni=1014,Bn=1015,Qr=1016,Wo=1017,qo=1018,es=1020,lu=35902,uu=35899,hu=1021,du=1022,Tn=1023,Gr=1026,ts=1027,Xo=1028,jo=1029,fu=1030,Yo=1031;var Zo=1033,ta=33776,na=33777,ia=33778,ra=33779,Ko=35840,Jo=35841,Qo=35842,ec=35843,tc=36196,nc=37492,ic=37496,rc=37808,sc=37809,ac=37810,oc=37811,cc=37812,lc=37813,uc=37814,hc=37815,dc=37816,fc=37817,pc=37818,mc=37819,gc=37820,_c=37821,yc=36492,vc=36494,xc=36495,Sc=36283,bc=36284,Mc=36285,Ec=36286;var Ns=2300,po=2301,co=2302,Xl=2400,jl=2401,Yl=2402;var cp=3200,lp=3201;var up=0,hp=1,di="",Vt="srgb",Qi="srgb-linear",Ls="linear",ot="srgb";var Yi=7680;var Zl=519,dp=512,fp=513,pp=514,pu=515,mp=516,gp=517,_p=518,yp=519,Kl=35044;var mu="300 es",Un=2e3,Us=2001;var Gn=class{addEventListener(e,t){this._listeners===void 0&&(this._listeners={});let i=this._listeners;i[e]===void 0&&(i[e]=[]),i[e].indexOf(t)===-1&&i[e].push(t)}hasEventListener(e,t){let i=this._listeners;return i===void 0?!1:i[e]!==void 0&&i[e].indexOf(t)!==-1}removeEventListener(e,t){let i=this._listeners;if(i===void 0)return;let r=i[e];if(r!==void 0){let s=r.indexOf(t);s!==-1&&r.splice(s,1)}}dispatchEvent(e){let t=this._listeners;if(t===void 0)return;let i=t[e.type];if(i!==void 0){e.target=this;let r=i.slice(0);for(let s=0,a=r.length;s<a;s++)r[s].call(this,e);e.target=null}}},kt=["00","01","02","03","04","05","06","07","08","09","0a","0b","0c","0d","0e","0f","10","11","12","13","14","15","16","17","18","19","1a","1b","1c","1d","1e","1f","20","21","22","23","24","25","26","27","28","29","2a","2b","2c","2d","2e","2f","30","31","32","33","34","35","36","37","38","39","3a","3b","3c","3d","3e","3f","40","41","42","43","44","45","46","47","48","49","4a","4b","4c","4d","4e","4f","50","51","52","53","54","55","56","57","58","59","5a","5b","5c","5d","5e","5f","60","61","62","63","64","65","66","67","68","69","6a","6b","6c","6d","6e","6f","70","71","72","73","74","75","76","77","78","79","7a","7b","7c","7d","7e","7f","80","81","82","83","84","85","86","87","88","89","8a","8b","8c","8d","8e","8f","90","91","92","93","94","95","96","97","98","99","9a","9b","9c","9d","9e","9f","a0","a1","a2","a3","a4","a5","a6","a7","a8","a9","aa","ab","ac","ad","ae","af","b0","b1","b2","b3","b4","b5","b6","b7","b8","b9","ba","bb","bc","bd","be","bf","c0","c1","c2","c3","c4","c5","c6","c7","c8","c9","ca","cb","cc","cd","ce","cf","d0","d1","d2","d3","d4","d5","d6","d7","d8","d9","da","db","dc","dd","de","df","e0","e1","e2","e3","e4","e5","e6","e7","e8","e9","ea","eb","ec","ed","ee","ef","f0","f1","f2","f3","f4","f5","f6","f7","f8","f9","fa","fb","fc","fd","fe","ff"],lf=1234567,Is=Math.PI/180,Wr=180/Math.PI;function ns(){let n=Math.random()*4294967295|0,e=Math.random()*4294967295|0,t=Math.random()*4294967295|0,i=Math.random()*4294967295|0;return(kt[n&255]+kt[n>>8&255]+kt[n>>16&255]+kt[n>>24&255]+"-"+kt[e&255]+kt[e>>8&255]+"-"+kt[e>>16&15|64]+kt[e>>24&255]+"-"+kt[t&63|128]+kt[t>>8&255]+"-"+kt[t>>16&255]+kt[t>>24&255]+kt[i&255]+kt[i>>8&255]+kt[i>>16&255]+kt[i>>24&255]).toLowerCase()}function We(n,e,t){return Math.max(e,Math.min(t,n))}function gu(n,e){return(n%e+e)%e}function Y0(n,e,t,i,r){return i+(n-e)*(r-i)/(t-e)}function Z0(n,e,t){return n!==e?(t-n)/(e-n):0}function Ds(n,e,t){return(1-t)*n+t*e}function K0(n,e,t,i){return Ds(n,e,1-Math.exp(-t*i))}function J0(n,e=1){return e-Math.abs(gu(n,e*2)-e)}function Q0(n,e,t){return n<=e?0:n>=t?1:(n=(n-e)/(t-e),n*n*(3-2*n))}function ey(n,e,t){return n<=e?0:n>=t?1:(n=(n-e)/(t-e),n*n*n*(n*(n*6-15)+10))}function ty(n,e){return n+Math.floor(Math.random()*(e-n+1))}function ny(n,e){return n+Math.random()*(e-n)}function iy(n){return n*(.5-Math.random())}function ry(n){n!==void 0&&(lf=n);let e=lf+=1831565813;return e=Math.imul(e^e>>>15,e|1),e^=e+Math.imul(e^e>>>7,e|61),((e^e>>>14)>>>0)/4294967296}function sy(n){return n*Is}function ay(n){return n*Wr}function oy(n){return(n&n-1)===0&&n!==0}function cy(n){return Math.pow(2,Math.ceil(Math.log(n)/Math.LN2))}function ly(n){return Math.pow(2,Math.floor(Math.log(n)/Math.LN2))}function uy(n,e,t,i,r){let s=Math.cos,a=Math.sin,o=s(t/2),c=a(t/2),u=s((e+i)/2),l=a((e+i)/2),h=s((e-i)/2),f=a((e-i)/2),p=s((i-e)/2),g=a((i-e)/2);switch(r){case"XYX":n.set(o*l,c*h,c*f,o*u);break;case"YZY":n.set(c*f,o*l,c*h,o*u);break;case"ZXZ":n.set(c*h,c*f,o*l,o*u);break;case"XZX":n.set(o*l,c*g,c*p,o*u);break;case"YXY":n.set(c*p,o*l,c*g,o*u);break;case"ZYZ":n.set(c*g,c*p,o*l,o*u);break;default:console.warn("THREE.MathUtils: .setQuaternionFromProperEuler() encountered an unknown order: "+r)}}function $r(n,e){switch(e.constructor){case Float32Array:return n;case Uint32Array:return n/4294967295;case Uint16Array:return n/65535;case Uint8Array:return n/255;case Int32Array:return Math.max(n/2147483647,-1);case Int16Array:return Math.max(n/32767,-1);case Int8Array:return Math.max(n/127,-1);default:throw new Error("Invalid component type.")}}function Kt(n,e){switch(e.constructor){case Float32Array:return n;case Uint32Array:return Math.round(n*4294967295);case Uint16Array:return Math.round(n*65535);case Uint8Array:return Math.round(n*255);case Int32Array:return Math.round(n*2147483647);case Int16Array:return Math.round(n*32767);case Int8Array:return Math.round(n*127);default:throw new Error("Invalid component type.")}}var sa={DEG2RAD:Is,RAD2DEG:Wr,generateUUID:ns,clamp:We,euclideanModulo:gu,mapLinear:Y0,inverseLerp:Z0,lerp:Ds,damp:K0,pingpong:J0,smoothstep:Q0,smootherstep:ey,randInt:ty,randFloat:ny,randFloatSpread:iy,seededRandom:ry,degToRad:sy,radToDeg:ay,isPowerOfTwo:oy,ceilPowerOfTwo:cy,floorPowerOfTwo:ly,setQuaternionFromProperEuler:uy,normalize:Kt,denormalize:$r},Ue=class n{constructor(e=0,t=0){n.prototype.isVector2=!0,this.x=e,this.y=t}get width(){return this.x}set width(e){this.x=e}get height(){return this.y}set height(e){this.y=e}set(e,t){return this.x=e,this.y=t,this}setScalar(e){return this.x=e,this.y=e,this}setX(e){return this.x=e,this}setY(e){return this.y=e,this}setComponent(e,t){switch(e){case 0:this.x=t;break;case 1:this.y=t;break;default:throw new Error("index is out of range: "+e)}return this}getComponent(e){switch(e){case 0:return this.x;case 1:return this.y;default:throw new Error("index is out of range: "+e)}}clone(){return new this.constructor(this.x,this.y)}copy(e){return this.x=e.x,this.y=e.y,this}add(e){return this.x+=e.x,this.y+=e.y,this}addScalar(e){return this.x+=e,this.y+=e,this}addVectors(e,t){return this.x=e.x+t.x,this.y=e.y+t.y,this}addScaledVector(e,t){return this.x+=e.x*t,this.y+=e.y*t,this}sub(e){return this.x-=e.x,this.y-=e.y,this}subScalar(e){return this.x-=e,this.y-=e,this}subVectors(e,t){return this.x=e.x-t.x,this.y=e.y-t.y,this}multiply(e){return this.x*=e.x,this.y*=e.y,this}multiplyScalar(e){return this.x*=e,this.y*=e,this}divide(e){return this.x/=e.x,this.y/=e.y,this}divideScalar(e){return this.multiplyScalar(1/e)}applyMatrix3(e){let t=this.x,i=this.y,r=e.elements;return this.x=r[0]*t+r[3]*i+r[6],this.y=r[1]*t+r[4]*i+r[7],this}min(e){return this.x=Math.min(this.x,e.x),this.y=Math.min(this.y,e.y),this}max(e){return this.x=Math.max(this.x,e.x),this.y=Math.max(this.y,e.y),this}clamp(e,t){return this.x=We(this.x,e.x,t.x),this.y=We(this.y,e.y,t.y),this}clampScalar(e,t){return this.x=We(this.x,e,t),this.y=We(this.y,e,t),this}clampLength(e,t){let i=this.length();return this.divideScalar(i||1).multiplyScalar(We(i,e,t))}floor(){return this.x=Math.floor(this.x),this.y=Math.floor(this.y),this}ceil(){return this.x=Math.ceil(this.x),this.y=Math.ceil(this.y),this}round(){return this.x=Math.round(this.x),this.y=Math.round(this.y),this}roundToZero(){return this.x=Math.trunc(this.x),this.y=Math.trunc(this.y),this}negate(){return this.x=-this.x,this.y=-this.y,this}dot(e){return this.x*e.x+this.y*e.y}cross(e){return this.x*e.y-this.y*e.x}lengthSq(){return this.x*this.x+this.y*this.y}length(){return Math.sqrt(this.x*this.x+this.y*this.y)}manhattanLength(){return Math.abs(this.x)+Math.abs(this.y)}normalize(){return this.divideScalar(this.length()||1)}angle(){return Math.atan2(-this.y,-this.x)+Math.PI}angleTo(e){let t=Math.sqrt(this.lengthSq()*e.lengthSq());if(t===0)return Math.PI/2;let i=this.dot(e)/t;return Math.acos(We(i,-1,1))}distanceTo(e){return Math.sqrt(this.distanceToSquared(e))}distanceToSquared(e){let t=this.x-e.x,i=this.y-e.y;return t*t+i*i}manhattanDistanceTo(e){return Math.abs(this.x-e.x)+Math.abs(this.y-e.y)}setLength(e){return this.normalize().multiplyScalar(e)}lerp(e,t){return this.x+=(e.x-this.x)*t,this.y+=(e.y-this.y)*t,this}lerpVectors(e,t,i){return this.x=e.x+(t.x-e.x)*i,this.y=e.y+(t.y-e.y)*i,this}equals(e){return e.x===this.x&&e.y===this.y}fromArray(e,t=0){return this.x=e[t],this.y=e[t+1],this}toArray(e=[],t=0){return e[t]=this.x,e[t+1]=this.y,e}fromBufferAttribute(e,t){return this.x=e.getX(t),this.y=e.getY(t),this}rotateAround(e,t){let i=Math.cos(t),r=Math.sin(t),s=this.x-e.x,a=this.y-e.y;return this.x=s*i-a*r+e.x,this.y=s*r+a*i+e.y,this}random(){return this.x=Math.random(),this.y=Math.random(),this}*[Symbol.iterator](){yield this.x,yield this.y}},an=class{constructor(e=0,t=0,i=0,r=1){this.isQuaternion=!0,this._x=e,this._y=t,this._z=i,this._w=r}static slerpFlat(e,t,i,r,s,a,o){let c=i[r+0],u=i[r+1],l=i[r+2],h=i[r+3],f=s[a+0],p=s[a+1],g=s[a+2],_=s[a+3];if(o===0){e[t+0]=c,e[t+1]=u,e[t+2]=l,e[t+3]=h;return}if(o===1){e[t+0]=f,e[t+1]=p,e[t+2]=g,e[t+3]=_;return}if(h!==_||c!==f||u!==p||l!==g){let m=1-o,d=c*f+u*p+l*g+h*_,w=d>=0?1:-1,M=1-d*d;if(M>Number.EPSILON){let T=Math.sqrt(M),R=Math.atan2(T,d*w);m=Math.sin(m*R)/T,o=Math.sin(o*R)/T}let x=o*w;if(c=c*m+f*x,u=u*m+p*x,l=l*m+g*x,h=h*m+_*x,m===1-o){let T=1/Math.sqrt(c*c+u*u+l*l+h*h);c*=T,u*=T,l*=T,h*=T}}e[t]=c,e[t+1]=u,e[t+2]=l,e[t+3]=h}static multiplyQuaternionsFlat(e,t,i,r,s,a){let o=i[r],c=i[r+1],u=i[r+2],l=i[r+3],h=s[a],f=s[a+1],p=s[a+2],g=s[a+3];return e[t]=o*g+l*h+c*p-u*f,e[t+1]=c*g+l*f+u*h-o*p,e[t+2]=u*g+l*p+o*f-c*h,e[t+3]=l*g-o*h-c*f-u*p,e}get x(){return this._x}set x(e){this._x=e,this._onChangeCallback()}get y(){return this._y}set y(e){this._y=e,this._onChangeCallback()}get z(){return this._z}set z(e){this._z=e,this._onChangeCallback()}get w(){return this._w}set w(e){this._w=e,this._onChangeCallback()}set(e,t,i,r){return this._x=e,this._y=t,this._z=i,this._w=r,this._onChangeCallback(),this}clone(){return new this.constructor(this._x,this._y,this._z,this._w)}copy(e){return this._x=e.x,this._y=e.y,this._z=e.z,this._w=e.w,this._onChangeCallback(),this}setFromEuler(e,t=!0){let i=e._x,r=e._y,s=e._z,a=e._order,o=Math.cos,c=Math.sin,u=o(i/2),l=o(r/2),h=o(s/2),f=c(i/2),p=c(r/2),g=c(s/2);switch(a){case"XYZ":this._x=f*l*h+u*p*g,this._y=u*p*h-f*l*g,this._z=u*l*g+f*p*h,this._w=u*l*h-f*p*g;break;case"YXZ":this._x=f*l*h+u*p*g,this._y=u*p*h-f*l*g,this._z=u*l*g-f*p*h,this._w=u*l*h+f*p*g;break;case"ZXY":this._x=f*l*h-u*p*g,this._y=u*p*h+f*l*g,this._z=u*l*g+f*p*h,this._w=u*l*h-f*p*g;break;case"ZYX":this._x=f*l*h-u*p*g,this._y=u*p*h+f*l*g,this._z=u*l*g-f*p*h,this._w=u*l*h+f*p*g;break;case"YZX":this._x=f*l*h+u*p*g,this._y=u*p*h+f*l*g,this._z=u*l*g-f*p*h,this._w=u*l*h-f*p*g;break;case"XZY":this._x=f*l*h-u*p*g,this._y=u*p*h-f*l*g,this._z=u*l*g+f*p*h,this._w=u*l*h+f*p*g;break;default:console.warn("THREE.Quaternion: .setFromEuler() encountered an unknown order: "+a)}return t===!0&&this._onChangeCallback(),this}setFromAxisAngle(e,t){let i=t/2,r=Math.sin(i);return this._x=e.x*r,this._y=e.y*r,this._z=e.z*r,this._w=Math.cos(i),this._onChangeCallback(),this}setFromRotationMatrix(e){let t=e.elements,i=t[0],r=t[4],s=t[8],a=t[1],o=t[5],c=t[9],u=t[2],l=t[6],h=t[10],f=i+o+h;if(f>0){let p=.5/Math.sqrt(f+1);this._w=.25/p,this._x=(l-c)*p,this._y=(s-u)*p,this._z=(a-r)*p}else if(i>o&&i>h){let p=2*Math.sqrt(1+i-o-h);this._w=(l-c)/p,this._x=.25*p,this._y=(r+a)/p,this._z=(s+u)/p}else if(o>h){let p=2*Math.sqrt(1+o-i-h);this._w=(s-u)/p,this._x=(r+a)/p,this._y=.25*p,this._z=(c+l)/p}else{let p=2*Math.sqrt(1+h-i-o);this._w=(a-r)/p,this._x=(s+u)/p,this._y=(c+l)/p,this._z=.25*p}return this._onChangeCallback(),this}setFromUnitVectors(e,t){let i=e.dot(t)+1;return i<1e-8?(i=0,Math.abs(e.x)>Math.abs(e.z)?(this._x=-e.y,this._y=e.x,this._z=0,this._w=i):(this._x=0,this._y=-e.z,this._z=e.y,this._w=i)):(this._x=e.y*t.z-e.z*t.y,this._y=e.z*t.x-e.x*t.z,this._z=e.x*t.y-e.y*t.x,this._w=i),this.normalize()}angleTo(e){return 2*Math.acos(Math.abs(We(this.dot(e),-1,1)))}rotateTowards(e,t){let i=this.angleTo(e);if(i===0)return this;let r=Math.min(1,t/i);return this.slerp(e,r),this}identity(){return this.set(0,0,0,1)}invert(){return this.conjugate()}conjugate(){return this._x*=-1,this._y*=-1,this._z*=-1,this._onChangeCallback(),this}dot(e){return this._x*e._x+this._y*e._y+this._z*e._z+this._w*e._w}lengthSq(){return this._x*this._x+this._y*this._y+this._z*this._z+this._w*this._w}length(){return Math.sqrt(this._x*this._x+this._y*this._y+this._z*this._z+this._w*this._w)}normalize(){let e=this.length();return e===0?(this._x=0,this._y=0,this._z=0,this._w=1):(e=1/e,this._x=this._x*e,this._y=this._y*e,this._z=this._z*e,this._w=this._w*e),this._onChangeCallback(),this}multiply(e){return this.multiplyQuaternions(this,e)}premultiply(e){return this.multiplyQuaternions(e,this)}multiplyQuaternions(e,t){let i=e._x,r=e._y,s=e._z,a=e._w,o=t._x,c=t._y,u=t._z,l=t._w;return this._x=i*l+a*o+r*u-s*c,this._y=r*l+a*c+s*o-i*u,this._z=s*l+a*u+i*c-r*o,this._w=a*l-i*o-r*c-s*u,this._onChangeCallback(),this}slerp(e,t){if(t===0)return this;if(t===1)return this.copy(e);let i=this._x,r=this._y,s=this._z,a=this._w,o=a*e._w+i*e._x+r*e._y+s*e._z;if(o<0?(this._w=-e._w,this._x=-e._x,this._y=-e._y,this._z=-e._z,o=-o):this.copy(e),o>=1)return this._w=a,this._x=i,this._y=r,this._z=s,this;let c=1-o*o;if(c<=Number.EPSILON){let p=1-t;return this._w=p*a+t*this._w,this._x=p*i+t*this._x,this._y=p*r+t*this._y,this._z=p*s+t*this._z,this.normalize(),this}let u=Math.sqrt(c),l=Math.atan2(u,o),h=Math.sin((1-t)*l)/u,f=Math.sin(t*l)/u;return this._w=a*h+this._w*f,this._x=i*h+this._x*f,this._y=r*h+this._y*f,this._z=s*h+this._z*f,this._onChangeCallback(),this}slerpQuaternions(e,t,i){return this.copy(e).slerp(t,i)}random(){let e=2*Math.PI*Math.random(),t=2*Math.PI*Math.random(),i=Math.random(),r=Math.sqrt(1-i),s=Math.sqrt(i);return this.set(r*Math.sin(e),r*Math.cos(e),s*Math.sin(t),s*Math.cos(t))}equals(e){return e._x===this._x&&e._y===this._y&&e._z===this._z&&e._w===this._w}fromArray(e,t=0){return this._x=e[t],this._y=e[t+1],this._z=e[t+2],this._w=e[t+3],this._onChangeCallback(),this}toArray(e=[],t=0){return e[t]=this._x,e[t+1]=this._y,e[t+2]=this._z,e[t+3]=this._w,e}fromBufferAttribute(e,t){return this._x=e.getX(t),this._y=e.getY(t),this._z=e.getZ(t),this._w=e.getW(t),this._onChangeCallback(),this}toJSON(){return this.toArray()}_onChange(e){return this._onChangeCallback=e,this}_onChangeCallback(){}*[Symbol.iterator](){yield this._x,yield this._y,yield this._z,yield this._w}},L=class n{constructor(e=0,t=0,i=0){n.prototype.isVector3=!0,this.x=e,this.y=t,this.z=i}set(e,t,i){return i===void 0&&(i=this.z),this.x=e,this.y=t,this.z=i,this}setScalar(e){return this.x=e,this.y=e,this.z=e,this}setX(e){return this.x=e,this}setY(e){return this.y=e,this}setZ(e){return this.z=e,this}setComponent(e,t){switch(e){case 0:this.x=t;break;case 1:this.y=t;break;case 2:this.z=t;break;default:throw new Error("index is out of range: "+e)}return this}getComponent(e){switch(e){case 0:return this.x;case 1:return this.y;case 2:return this.z;default:throw new Error("index is out of range: "+e)}}clone(){return new this.constructor(this.x,this.y,this.z)}copy(e){return this.x=e.x,this.y=e.y,this.z=e.z,this}add(e){return this.x+=e.x,this.y+=e.y,this.z+=e.z,this}addScalar(e){return this.x+=e,this.y+=e,this.z+=e,this}addVectors(e,t){return this.x=e.x+t.x,this.y=e.y+t.y,this.z=e.z+t.z,this}addScaledVector(e,t){return this.x+=e.x*t,this.y+=e.y*t,this.z+=e.z*t,this}sub(e){return this.x-=e.x,this.y-=e.y,this.z-=e.z,this}subScalar(e){return this.x-=e,this.y-=e,this.z-=e,this}subVectors(e,t){return this.x=e.x-t.x,this.y=e.y-t.y,this.z=e.z-t.z,this}multiply(e){return this.x*=e.x,this.y*=e.y,this.z*=e.z,this}multiplyScalar(e){return this.x*=e,this.y*=e,this.z*=e,this}multiplyVectors(e,t){return this.x=e.x*t.x,this.y=e.y*t.y,this.z=e.z*t.z,this}applyEuler(e){return this.applyQuaternion(uf.setFromEuler(e))}applyAxisAngle(e,t){return this.applyQuaternion(uf.setFromAxisAngle(e,t))}applyMatrix3(e){let t=this.x,i=this.y,r=this.z,s=e.elements;return this.x=s[0]*t+s[3]*i+s[6]*r,this.y=s[1]*t+s[4]*i+s[7]*r,this.z=s[2]*t+s[5]*i+s[8]*r,this}applyNormalMatrix(e){return this.applyMatrix3(e).normalize()}applyMatrix4(e){let t=this.x,i=this.y,r=this.z,s=e.elements,a=1/(s[3]*t+s[7]*i+s[11]*r+s[15]);return this.x=(s[0]*t+s[4]*i+s[8]*r+s[12])*a,this.y=(s[1]*t+s[5]*i+s[9]*r+s[13])*a,this.z=(s[2]*t+s[6]*i+s[10]*r+s[14])*a,this}applyQuaternion(e){let t=this.x,i=this.y,r=this.z,s=e.x,a=e.y,o=e.z,c=e.w,u=2*(a*r-o*i),l=2*(o*t-s*r),h=2*(s*i-a*t);return this.x=t+c*u+a*h-o*l,this.y=i+c*l+o*u-s*h,this.z=r+c*h+s*l-a*u,this}project(e){return this.applyMatrix4(e.matrixWorldInverse).applyMatrix4(e.projectionMatrix)}unproject(e){return this.applyMatrix4(e.projectionMatrixInverse).applyMatrix4(e.matrixWorld)}transformDirection(e){let t=this.x,i=this.y,r=this.z,s=e.elements;return this.x=s[0]*t+s[4]*i+s[8]*r,this.y=s[1]*t+s[5]*i+s[9]*r,this.z=s[2]*t+s[6]*i+s[10]*r,this.normalize()}divide(e){return this.x/=e.x,this.y/=e.y,this.z/=e.z,this}divideScalar(e){return this.multiplyScalar(1/e)}min(e){return this.x=Math.min(this.x,e.x),this.y=Math.min(this.y,e.y),this.z=Math.min(this.z,e.z),this}max(e){return this.x=Math.max(this.x,e.x),this.y=Math.max(this.y,e.y),this.z=Math.max(this.z,e.z),this}clamp(e,t){return this.x=We(this.x,e.x,t.x),this.y=We(this.y,e.y,t.y),this.z=We(this.z,e.z,t.z),this}clampScalar(e,t){return this.x=We(this.x,e,t),this.y=We(this.y,e,t),this.z=We(this.z,e,t),this}clampLength(e,t){let i=this.length();return this.divideScalar(i||1).multiplyScalar(We(i,e,t))}floor(){return this.x=Math.floor(this.x),this.y=Math.floor(this.y),this.z=Math.floor(this.z),this}ceil(){return this.x=Math.ceil(this.x),this.y=Math.ceil(this.y),this.z=Math.ceil(this.z),this}round(){return this.x=Math.round(this.x),this.y=Math.round(this.y),this.z=Math.round(this.z),this}roundToZero(){return this.x=Math.trunc(this.x),this.y=Math.trunc(this.y),this.z=Math.trunc(this.z),this}negate(){return this.x=-this.x,this.y=-this.y,this.z=-this.z,this}dot(e){return this.x*e.x+this.y*e.y+this.z*e.z}lengthSq(){return this.x*this.x+this.y*this.y+this.z*this.z}length(){return Math.sqrt(this.x*this.x+this.y*this.y+this.z*this.z)}manhattanLength(){return Math.abs(this.x)+Math.abs(this.y)+Math.abs(this.z)}normalize(){return this.divideScalar(this.length()||1)}setLength(e){return this.normalize().multiplyScalar(e)}lerp(e,t){return this.x+=(e.x-this.x)*t,this.y+=(e.y-this.y)*t,this.z+=(e.z-this.z)*t,this}lerpVectors(e,t,i){return this.x=e.x+(t.x-e.x)*i,this.y=e.y+(t.y-e.y)*i,this.z=e.z+(t.z-e.z)*i,this}cross(e){return this.crossVectors(this,e)}crossVectors(e,t){let i=e.x,r=e.y,s=e.z,a=t.x,o=t.y,c=t.z;return this.x=r*c-s*o,this.y=s*a-i*c,this.z=i*o-r*a,this}projectOnVector(e){let t=e.lengthSq();if(t===0)return this.set(0,0,0);let i=e.dot(this)/t;return this.copy(e).multiplyScalar(i)}projectOnPlane(e){return Tl.copy(this).projectOnVector(e),this.sub(Tl)}reflect(e){return this.sub(Tl.copy(e).multiplyScalar(2*this.dot(e)))}angleTo(e){let t=Math.sqrt(this.lengthSq()*e.lengthSq());if(t===0)return Math.PI/2;let i=this.dot(e)/t;return Math.acos(We(i,-1,1))}distanceTo(e){return Math.sqrt(this.distanceToSquared(e))}distanceToSquared(e){let t=this.x-e.x,i=this.y-e.y,r=this.z-e.z;return t*t+i*i+r*r}manhattanDistanceTo(e){return Math.abs(this.x-e.x)+Math.abs(this.y-e.y)+Math.abs(this.z-e.z)}setFromSpherical(e){return this.setFromSphericalCoords(e.radius,e.phi,e.theta)}setFromSphericalCoords(e,t,i){let r=Math.sin(t)*e;return this.x=r*Math.sin(i),this.y=Math.cos(t)*e,this.z=r*Math.cos(i),this}setFromCylindrical(e){return this.setFromCylindricalCoords(e.radius,e.theta,e.y)}setFromCylindricalCoords(e,t,i){return this.x=e*Math.sin(t),this.y=i,this.z=e*Math.cos(t),this}setFromMatrixPosition(e){let t=e.elements;return this.x=t[12],this.y=t[13],this.z=t[14],this}setFromMatrixScale(e){let t=this.setFromMatrixColumn(e,0).length(),i=this.setFromMatrixColumn(e,1).length(),r=this.setFromMatrixColumn(e,2).length();return this.x=t,this.y=i,this.z=r,this}setFromMatrixColumn(e,t){return this.fromArray(e.elements,t*4)}setFromMatrix3Column(e,t){return this.fromArray(e.elements,t*3)}setFromEuler(e){return this.x=e._x,this.y=e._y,this.z=e._z,this}setFromColor(e){return this.x=e.r,this.y=e.g,this.z=e.b,this}equals(e){return e.x===this.x&&e.y===this.y&&e.z===this.z}fromArray(e,t=0){return this.x=e[t],this.y=e[t+1],this.z=e[t+2],this}toArray(e=[],t=0){return e[t]=this.x,e[t+1]=this.y,e[t+2]=this.z,e}fromBufferAttribute(e,t){return this.x=e.getX(t),this.y=e.getY(t),this.z=e.getZ(t),this}random(){return this.x=Math.random(),this.y=Math.random(),this.z=Math.random(),this}randomDirection(){let e=Math.random()*Math.PI*2,t=Math.random()*2-1,i=Math.sqrt(1-t*t);return this.x=i*Math.cos(e),this.y=t,this.z=i*Math.sin(e),this}*[Symbol.iterator](){yield this.x,yield this.y,yield this.z}},Tl=new L,uf=new an,Fe=class n{constructor(e,t,i,r,s,a,o,c,u){n.prototype.isMatrix3=!0,this.elements=[1,0,0,0,1,0,0,0,1],e!==void 0&&this.set(e,t,i,r,s,a,o,c,u)}set(e,t,i,r,s,a,o,c,u){let l=this.elements;return l[0]=e,l[1]=r,l[2]=o,l[3]=t,l[4]=s,l[5]=c,l[6]=i,l[7]=a,l[8]=u,this}identity(){return this.set(1,0,0,0,1,0,0,0,1),this}copy(e){let t=this.elements,i=e.elements;return t[0]=i[0],t[1]=i[1],t[2]=i[2],t[3]=i[3],t[4]=i[4],t[5]=i[5],t[6]=i[6],t[7]=i[7],t[8]=i[8],this}extractBasis(e,t,i){return e.setFromMatrix3Column(this,0),t.setFromMatrix3Column(this,1),i.setFromMatrix3Column(this,2),this}setFromMatrix4(e){let t=e.elements;return this.set(t[0],t[4],t[8],t[1],t[5],t[9],t[2],t[6],t[10]),this}multiply(e){return this.multiplyMatrices(this,e)}premultiply(e){return this.multiplyMatrices(e,this)}multiplyMatrices(e,t){let i=e.elements,r=t.elements,s=this.elements,a=i[0],o=i[3],c=i[6],u=i[1],l=i[4],h=i[7],f=i[2],p=i[5],g=i[8],_=r[0],m=r[3],d=r[6],w=r[1],M=r[4],x=r[7],T=r[2],R=r[5],P=r[8];return s[0]=a*_+o*w+c*T,s[3]=a*m+o*M+c*R,s[6]=a*d+o*x+c*P,s[1]=u*_+l*w+h*T,s[4]=u*m+l*M+h*R,s[7]=u*d+l*x+h*P,s[2]=f*_+p*w+g*T,s[5]=f*m+p*M+g*R,s[8]=f*d+p*x+g*P,this}multiplyScalar(e){let t=this.elements;return t[0]*=e,t[3]*=e,t[6]*=e,t[1]*=e,t[4]*=e,t[7]*=e,t[2]*=e,t[5]*=e,t[8]*=e,this}determinant(){let e=this.elements,t=e[0],i=e[1],r=e[2],s=e[3],a=e[4],o=e[5],c=e[6],u=e[7],l=e[8];return t*a*l-t*o*u-i*s*l+i*o*c+r*s*u-r*a*c}invert(){let e=this.elements,t=e[0],i=e[1],r=e[2],s=e[3],a=e[4],o=e[5],c=e[6],u=e[7],l=e[8],h=l*a-o*u,f=o*c-l*s,p=u*s-a*c,g=t*h+i*f+r*p;if(g===0)return this.set(0,0,0,0,0,0,0,0,0);let _=1/g;return e[0]=h*_,e[1]=(r*u-l*i)*_,e[2]=(o*i-r*a)*_,e[3]=f*_,e[4]=(l*t-r*c)*_,e[5]=(r*s-o*t)*_,e[6]=p*_,e[7]=(i*c-u*t)*_,e[8]=(a*t-i*s)*_,this}transpose(){let e,t=this.elements;return e=t[1],t[1]=t[3],t[3]=e,e=t[2],t[2]=t[6],t[6]=e,e=t[5],t[5]=t[7],t[7]=e,this}getNormalMatrix(e){return this.setFromMatrix4(e).invert().transpose()}transposeIntoArray(e){let t=this.elements;return e[0]=t[0],e[1]=t[3],e[2]=t[6],e[3]=t[1],e[4]=t[4],e[5]=t[7],e[6]=t[2],e[7]=t[5],e[8]=t[8],this}setUvTransform(e,t,i,r,s,a,o){let c=Math.cos(s),u=Math.sin(s);return this.set(i*c,i*u,-i*(c*a+u*o)+a+e,-r*u,r*c,-r*(-u*a+c*o)+o+t,0,0,1),this}scale(e,t){return this.premultiply(Al.makeScale(e,t)),this}rotate(e){return this.premultiply(Al.makeRotation(-e)),this}translate(e,t){return this.premultiply(Al.makeTranslation(e,t)),this}makeTranslation(e,t){return e.isVector2?this.set(1,0,e.x,0,1,e.y,0,0,1):this.set(1,0,e,0,1,t,0,0,1),this}makeRotation(e){let t=Math.cos(e),i=Math.sin(e);return this.set(t,-i,0,i,t,0,0,0,1),this}makeScale(e,t){return this.set(e,0,0,0,t,0,0,0,1),this}equals(e){let t=this.elements,i=e.elements;for(let r=0;r<9;r++)if(t[r]!==i[r])return!1;return!0}fromArray(e,t=0){for(let i=0;i<9;i++)this.elements[i]=e[i+t];return this}toArray(e=[],t=0){let i=this.elements;return e[t]=i[0],e[t+1]=i[1],e[t+2]=i[2],e[t+3]=i[3],e[t+4]=i[4],e[t+5]=i[5],e[t+6]=i[6],e[t+7]=i[7],e[t+8]=i[8],e}clone(){return new this.constructor().fromArray(this.elements)}},Al=new Fe;function _u(n){for(let e=n.length-1;e>=0;--e)if(n[e]>=65535)return!0;return!1}function Os(n){return document.createElementNS("http://www.w3.org/1999/xhtml",n)}function vp(){let n=Os("canvas");return n.style.display="block",n}var hf={};function qr(n){n in hf||(hf[n]=!0,console.warn(n))}function xp(n,e,t){return new Promise(function(i,r){function s(){switch(n.clientWaitSync(e,n.SYNC_FLUSH_COMMANDS_BIT,0)){case n.WAIT_FAILED:r();break;case n.TIMEOUT_EXPIRED:setTimeout(s,t);break;default:i()}}setTimeout(s,t)})}var df=new Fe().set(.4123908,.3575843,.1804808,.212639,.7151687,.0721923,.0193308,.1191948,.9505322),ff=new Fe().set(3.2409699,-1.5373832,-.4986108,-.9692436,1.8759675,.0415551,.0556301,-.203977,1.0569715);function hy(){let n={enabled:!0,workingColorSpace:Qi,spaces:{},convert:function(r,s,a){return this.enabled===!1||s===a||!s||!a||(this.spaces[s].transfer===ot&&(r.r=oi(r.r),r.g=oi(r.g),r.b=oi(r.b)),this.spaces[s].primaries!==this.spaces[a].primaries&&(r.applyMatrix3(this.spaces[s].toXYZ),r.applyMatrix3(this.spaces[a].fromXYZ)),this.spaces[a].transfer===ot&&(r.r=Hr(r.r),r.g=Hr(r.g),r.b=Hr(r.b))),r},workingToColorSpace:function(r,s){return this.convert(r,this.workingColorSpace,s)},colorSpaceToWorking:function(r,s){return this.convert(r,s,this.workingColorSpace)},getPrimaries:function(r){return this.spaces[r].primaries},getTransfer:function(r){return r===di?Ls:this.spaces[r].transfer},getToneMappingMode:function(r){return this.spaces[r].outputColorSpaceConfig.toneMappingMode||"standard"},getLuminanceCoefficients:function(r,s=this.workingColorSpace){return r.fromArray(this.spaces[s].luminanceCoefficients)},define:function(r){Object.assign(this.spaces,r)},_getMatrix:function(r,s,a){return r.copy(this.spaces[s].toXYZ).multiply(this.spaces[a].fromXYZ)},_getDrawingBufferColorSpace:function(r){return this.spaces[r].outputColorSpaceConfig.drawingBufferColorSpace},_getUnpackColorSpace:function(r=this.workingColorSpace){return this.spaces[r].workingColorSpaceConfig.unpackColorSpace},fromWorkingColorSpace:function(r,s){return qr("THREE.ColorManagement: .fromWorkingColorSpace() has been renamed to .workingToColorSpace()."),n.workingToColorSpace(r,s)},toWorkingColorSpace:function(r,s){return qr("THREE.ColorManagement: .toWorkingColorSpace() has been renamed to .colorSpaceToWorking()."),n.colorSpaceToWorking(r,s)}},e=[.64,.33,.3,.6,.15,.06],t=[.2126,.7152,.0722],i=[.3127,.329];return n.define({[Qi]:{primaries:e,whitePoint:i,transfer:Ls,toXYZ:df,fromXYZ:ff,luminanceCoefficients:t,workingColorSpaceConfig:{unpackColorSpace:Vt},outputColorSpaceConfig:{drawingBufferColorSpace:Vt}},[Vt]:{primaries:e,whitePoint:i,transfer:ot,toXYZ:df,fromXYZ:ff,luminanceCoefficients:t,outputColorSpaceConfig:{drawingBufferColorSpace:Vt}}}),n}var tt=hy();function oi(n){return n<.04045?n*.0773993808:Math.pow(n*.9478672986+.0521327014,2.4)}function Hr(n){return n<.0031308?n*12.92:1.055*Math.pow(n,.41666)-.055}var Cr,mo=class{static getDataURL(e,t="image/png"){if(/^data:/i.test(e.src)||typeof HTMLCanvasElement>"u")return e.src;let i;if(e instanceof HTMLCanvasElement)i=e;else{Cr===void 0&&(Cr=Os("canvas")),Cr.width=e.width,Cr.height=e.height;let r=Cr.getContext("2d");e instanceof ImageData?r.putImageData(e,0,0):r.drawImage(e,0,0,e.width,e.height),i=Cr}return i.toDataURL(t)}static sRGBToLinear(e){if(typeof HTMLImageElement<"u"&&e instanceof HTMLImageElement||typeof HTMLCanvasElement<"u"&&e instanceof HTMLCanvasElement||typeof ImageBitmap<"u"&&e instanceof ImageBitmap){let t=Os("canvas");t.width=e.width,t.height=e.height;let i=t.getContext("2d");i.drawImage(e,0,0,e.width,e.height);let r=i.getImageData(0,0,e.width,e.height),s=r.data;for(let a=0;a<s.length;a++)s[a]=oi(s[a]/255)*255;return i.putImageData(r,0,0),t}else if(e.data){let t=e.data.slice(0);for(let i=0;i<t.length;i++)t instanceof Uint8Array||t instanceof Uint8ClampedArray?t[i]=Math.floor(oi(t[i]/255)*255):t[i]=oi(t[i]);return{data:t,width:e.width,height:e.height}}else return console.warn("THREE.ImageUtils.sRGBToLinear(): Unsupported image type. No color space conversion applied."),e}},dy=0,Xr=class{constructor(e=null){this.isSource=!0,Object.defineProperty(this,"id",{value:dy++}),this.uuid=ns(),this.data=e,this.dataReady=!0,this.version=0}getSize(e){let t=this.data;return typeof HTMLVideoElement<"u"&&t instanceof HTMLVideoElement?e.set(t.videoWidth,t.videoHeight,0):t instanceof VideoFrame?e.set(t.displayHeight,t.displayWidth,0):t!==null?e.set(t.width,t.height,t.depth||0):e.set(0,0,0),e}set needsUpdate(e){e===!0&&this.version++}toJSON(e){let t=e===void 0||typeof e=="string";if(!t&&e.images[this.uuid]!==void 0)return e.images[this.uuid];let i={uuid:this.uuid,url:""},r=this.data;if(r!==null){let s;if(Array.isArray(r)){s=[];for(let a=0,o=r.length;a<o;a++)r[a].isDataTexture?s.push(Rl(r[a].image)):s.push(Rl(r[a]))}else s=Rl(r);i.url=s}return t||(e.images[this.uuid]=i),i}};function Rl(n){return typeof HTMLImageElement<"u"&&n instanceof HTMLImageElement||typeof HTMLCanvasElement<"u"&&n instanceof HTMLCanvasElement||typeof ImageBitmap<"u"&&n instanceof ImageBitmap?mo.getDataURL(n):n.data?{data:Array.from(n.data),width:n.width,height:n.height,type:n.data.constructor.name}:(console.warn("THREE.Texture: Unable to serialize Texture."),{})}var fy=0,Cl=new L,on=class n extends Gn{constructor(e=n.DEFAULT_IMAGE,t=n.DEFAULT_MAPPING,i=Ei,r=Ei,s=On,a=Di,o=Tn,c=Zn,u=n.DEFAULT_ANISOTROPY,l=di){super(),this.isTexture=!0,Object.defineProperty(this,"id",{value:fy++}),this.uuid=ns(),this.name="",this.source=new Xr(e),this.mipmaps=[],this.mapping=t,this.channel=0,this.wrapS=i,this.wrapT=r,this.magFilter=s,this.minFilter=a,this.anisotropy=u,this.format=o,this.internalFormat=null,this.type=c,this.offset=new Ue(0,0),this.repeat=new Ue(1,1),this.center=new Ue(0,0),this.rotation=0,this.matrixAutoUpdate=!0,this.matrix=new Fe,this.generateMipmaps=!0,this.premultiplyAlpha=!1,this.flipY=!0,this.unpackAlignment=4,this.colorSpace=l,this.userData={},this.updateRanges=[],this.version=0,this.onUpdate=null,this.renderTarget=null,this.isRenderTargetTexture=!1,this.isArrayTexture=!!(e&&e.depth&&e.depth>1),this.pmremVersion=0}get width(){return this.source.getSize(Cl).x}get height(){return this.source.getSize(Cl).y}get depth(){return this.source.getSize(Cl).z}get image(){return this.source.data}set image(e=null){this.source.data=e}updateMatrix(){this.matrix.setUvTransform(this.offset.x,this.offset.y,this.repeat.x,this.repeat.y,this.rotation,this.center.x,this.center.y)}addUpdateRange(e,t){this.updateRanges.push({start:e,count:t})}clearUpdateRanges(){this.updateRanges.length=0}clone(){return new this.constructor().copy(this)}copy(e){return this.name=e.name,this.source=e.source,this.mipmaps=e.mipmaps.slice(0),this.mapping=e.mapping,this.channel=e.channel,this.wrapS=e.wrapS,this.wrapT=e.wrapT,this.magFilter=e.magFilter,this.minFilter=e.minFilter,this.anisotropy=e.anisotropy,this.format=e.format,this.internalFormat=e.internalFormat,this.type=e.type,this.offset.copy(e.offset),this.repeat.copy(e.repeat),this.center.copy(e.center),this.rotation=e.rotation,this.matrixAutoUpdate=e.matrixAutoUpdate,this.matrix.copy(e.matrix),this.generateMipmaps=e.generateMipmaps,this.premultiplyAlpha=e.premultiplyAlpha,this.flipY=e.flipY,this.unpackAlignment=e.unpackAlignment,this.colorSpace=e.colorSpace,this.renderTarget=e.renderTarget,this.isRenderTargetTexture=e.isRenderTargetTexture,this.isArrayTexture=e.isArrayTexture,this.userData=JSON.parse(JSON.stringify(e.userData)),this.needsUpdate=!0,this}setValues(e){for(let t in e){let i=e[t];if(i===void 0){console.warn(`THREE.Texture.setValues(): parameter '${t}' has value of undefined.`);continue}let r=this[t];if(r===void 0){console.warn(`THREE.Texture.setValues(): property '${t}' does not exist.`);continue}r&&i&&r.isVector2&&i.isVector2||r&&i&&r.isVector3&&i.isVector3||r&&i&&r.isMatrix3&&i.isMatrix3?r.copy(i):this[t]=i}}toJSON(e){let t=e===void 0||typeof e=="string";if(!t&&e.textures[this.uuid]!==void 0)return e.textures[this.uuid];let i={metadata:{version:4.7,type:"Texture",generator:"Texture.toJSON"},uuid:this.uuid,name:this.name,image:this.source.toJSON(e).uuid,mapping:this.mapping,channel:this.channel,repeat:[this.repeat.x,this.repeat.y],offset:[this.offset.x,this.offset.y],center:[this.center.x,this.center.y],rotation:this.rotation,wrap:[this.wrapS,this.wrapT],format:this.format,internalFormat:this.internalFormat,type:this.type,colorSpace:this.colorSpace,minFilter:this.minFilter,magFilter:this.magFilter,anisotropy:this.anisotropy,flipY:this.flipY,generateMipmaps:this.generateMipmaps,premultiplyAlpha:this.premultiplyAlpha,unpackAlignment:this.unpackAlignment};return Object.keys(this.userData).length>0&&(i.userData=this.userData),t||(e.textures[this.uuid]=i),i}dispose(){this.dispatchEvent({type:"dispose"})}transformUv(e){if(this.mapping!==au)return e;if(e.applyMatrix3(this.matrix),e.x<0||e.x>1)switch(this.wrapS){case ho:e.x=e.x-Math.floor(e.x);break;case Ei:e.x=e.x<0?0:1;break;case fo:Math.abs(Math.floor(e.x)%2)===1?e.x=Math.ceil(e.x)-e.x:e.x=e.x-Math.floor(e.x);break}if(e.y<0||e.y>1)switch(this.wrapT){case ho:e.y=e.y-Math.floor(e.y);break;case Ei:e.y=e.y<0?0:1;break;case fo:Math.abs(Math.floor(e.y)%2)===1?e.y=Math.ceil(e.y)-e.y:e.y=e.y-Math.floor(e.y);break}return this.flipY&&(e.y=1-e.y),e}set needsUpdate(e){e===!0&&(this.version++,this.source.needsUpdate=!0)}set needsPMREMUpdate(e){e===!0&&this.pmremVersion++}};on.DEFAULT_IMAGE=null;on.DEFAULT_MAPPING=au;on.DEFAULT_ANISOTROPY=1;var Tt=class n{constructor(e=0,t=0,i=0,r=1){n.prototype.isVector4=!0,this.x=e,this.y=t,this.z=i,this.w=r}get width(){return this.z}set width(e){this.z=e}get height(){return this.w}set height(e){this.w=e}set(e,t,i,r){return this.x=e,this.y=t,this.z=i,this.w=r,this}setScalar(e){return this.x=e,this.y=e,this.z=e,this.w=e,this}setX(e){return this.x=e,this}setY(e){return this.y=e,this}setZ(e){return this.z=e,this}setW(e){return this.w=e,this}setComponent(e,t){switch(e){case 0:this.x=t;break;case 1:this.y=t;break;case 2:this.z=t;break;case 3:this.w=t;break;default:throw new Error("index is out of range: "+e)}return this}getComponent(e){switch(e){case 0:return this.x;case 1:return this.y;case 2:return this.z;case 3:return this.w;default:throw new Error("index is out of range: "+e)}}clone(){return new this.constructor(this.x,this.y,this.z,this.w)}copy(e){return this.x=e.x,this.y=e.y,this.z=e.z,this.w=e.w!==void 0?e.w:1,this}add(e){return this.x+=e.x,this.y+=e.y,this.z+=e.z,this.w+=e.w,this}addScalar(e){return this.x+=e,this.y+=e,this.z+=e,this.w+=e,this}addVectors(e,t){return this.x=e.x+t.x,this.y=e.y+t.y,this.z=e.z+t.z,this.w=e.w+t.w,this}addScaledVector(e,t){return this.x+=e.x*t,this.y+=e.y*t,this.z+=e.z*t,this.w+=e.w*t,this}sub(e){return this.x-=e.x,this.y-=e.y,this.z-=e.z,this.w-=e.w,this}subScalar(e){return this.x-=e,this.y-=e,this.z-=e,this.w-=e,this}subVectors(e,t){return this.x=e.x-t.x,this.y=e.y-t.y,this.z=e.z-t.z,this.w=e.w-t.w,this}multiply(e){return this.x*=e.x,this.y*=e.y,this.z*=e.z,this.w*=e.w,this}multiplyScalar(e){return this.x*=e,this.y*=e,this.z*=e,this.w*=e,this}applyMatrix4(e){let t=this.x,i=this.y,r=this.z,s=this.w,a=e.elements;return this.x=a[0]*t+a[4]*i+a[8]*r+a[12]*s,this.y=a[1]*t+a[5]*i+a[9]*r+a[13]*s,this.z=a[2]*t+a[6]*i+a[10]*r+a[14]*s,this.w=a[3]*t+a[7]*i+a[11]*r+a[15]*s,this}divide(e){return this.x/=e.x,this.y/=e.y,this.z/=e.z,this.w/=e.w,this}divideScalar(e){return this.multiplyScalar(1/e)}setAxisAngleFromQuaternion(e){this.w=2*Math.acos(e.w);let t=Math.sqrt(1-e.w*e.w);return t<1e-4?(this.x=1,this.y=0,this.z=0):(this.x=e.x/t,this.y=e.y/t,this.z=e.z/t),this}setAxisAngleFromRotationMatrix(e){let t,i,r,s,c=e.elements,u=c[0],l=c[4],h=c[8],f=c[1],p=c[5],g=c[9],_=c[2],m=c[6],d=c[10];if(Math.abs(l-f)<.01&&Math.abs(h-_)<.01&&Math.abs(g-m)<.01){if(Math.abs(l+f)<.1&&Math.abs(h+_)<.1&&Math.abs(g+m)<.1&&Math.abs(u+p+d-3)<.1)return this.set(1,0,0,0),this;t=Math.PI;let M=(u+1)/2,x=(p+1)/2,T=(d+1)/2,R=(l+f)/4,P=(h+_)/4,O=(g+m)/4;return M>x&&M>T?M<.01?(i=0,r=.707106781,s=.707106781):(i=Math.sqrt(M),r=R/i,s=P/i):x>T?x<.01?(i=.707106781,r=0,s=.707106781):(r=Math.sqrt(x),i=R/r,s=O/r):T<.01?(i=.707106781,r=.707106781,s=0):(s=Math.sqrt(T),i=P/s,r=O/s),this.set(i,r,s,t),this}let w=Math.sqrt((m-g)*(m-g)+(h-_)*(h-_)+(f-l)*(f-l));return Math.abs(w)<.001&&(w=1),this.x=(m-g)/w,this.y=(h-_)/w,this.z=(f-l)/w,this.w=Math.acos((u+p+d-1)/2),this}setFromMatrixPosition(e){let t=e.elements;return this.x=t[12],this.y=t[13],this.z=t[14],this.w=t[15],this}min(e){return this.x=Math.min(this.x,e.x),this.y=Math.min(this.y,e.y),this.z=Math.min(this.z,e.z),this.w=Math.min(this.w,e.w),this}max(e){return this.x=Math.max(this.x,e.x),this.y=Math.max(this.y,e.y),this.z=Math.max(this.z,e.z),this.w=Math.max(this.w,e.w),this}clamp(e,t){return this.x=We(this.x,e.x,t.x),this.y=We(this.y,e.y,t.y),this.z=We(this.z,e.z,t.z),this.w=We(this.w,e.w,t.w),this}clampScalar(e,t){return this.x=We(this.x,e,t),this.y=We(this.y,e,t),this.z=We(this.z,e,t),this.w=We(this.w,e,t),this}clampLength(e,t){let i=this.length();return this.divideScalar(i||1).multiplyScalar(We(i,e,t))}floor(){return this.x=Math.floor(this.x),this.y=Math.floor(this.y),this.z=Math.floor(this.z),this.w=Math.floor(this.w),this}ceil(){return this.x=Math.ceil(this.x),this.y=Math.ceil(this.y),this.z=Math.ceil(this.z),this.w=Math.ceil(this.w),this}round(){return this.x=Math.round(this.x),this.y=Math.round(this.y),this.z=Math.round(this.z),this.w=Math.round(this.w),this}roundToZero(){return this.x=Math.trunc(this.x),this.y=Math.trunc(this.y),this.z=Math.trunc(this.z),this.w=Math.trunc(this.w),this}negate(){return this.x=-this.x,this.y=-this.y,this.z=-this.z,this.w=-this.w,this}dot(e){return this.x*e.x+this.y*e.y+this.z*e.z+this.w*e.w}lengthSq(){return this.x*this.x+this.y*this.y+this.z*this.z+this.w*this.w}length(){return Math.sqrt(this.x*this.x+this.y*this.y+this.z*this.z+this.w*this.w)}manhattanLength(){return Math.abs(this.x)+Math.abs(this.y)+Math.abs(this.z)+Math.abs(this.w)}normalize(){return this.divideScalar(this.length()||1)}setLength(e){return this.normalize().multiplyScalar(e)}lerp(e,t){return this.x+=(e.x-this.x)*t,this.y+=(e.y-this.y)*t,this.z+=(e.z-this.z)*t,this.w+=(e.w-this.w)*t,this}lerpVectors(e,t,i){return this.x=e.x+(t.x-e.x)*i,this.y=e.y+(t.y-e.y)*i,this.z=e.z+(t.z-e.z)*i,this.w=e.w+(t.w-e.w)*i,this}equals(e){return e.x===this.x&&e.y===this.y&&e.z===this.z&&e.w===this.w}fromArray(e,t=0){return this.x=e[t],this.y=e[t+1],this.z=e[t+2],this.w=e[t+3],this}toArray(e=[],t=0){return e[t]=this.x,e[t+1]=this.y,e[t+2]=this.z,e[t+3]=this.w,e}fromBufferAttribute(e,t){return this.x=e.getX(t),this.y=e.getY(t),this.z=e.getZ(t),this.w=e.getW(t),this}random(){return this.x=Math.random(),this.y=Math.random(),this.z=Math.random(),this.w=Math.random(),this}*[Symbol.iterator](){yield this.x,yield this.y,yield this.z,yield this.w}},go=class extends Gn{constructor(e=1,t=1,i={}){super(),i=Object.assign({generateMipmaps:!1,internalFormat:null,minFilter:On,depthBuffer:!0,stencilBuffer:!1,resolveDepthBuffer:!0,resolveStencilBuffer:!0,depthTexture:null,samples:0,count:1,depth:1,multiview:!1},i),this.isRenderTarget=!0,this.width=e,this.height=t,this.depth=i.depth,this.scissor=new Tt(0,0,e,t),this.scissorTest=!1,this.viewport=new Tt(0,0,e,t);let r={width:e,height:t,depth:i.depth},s=new on(r);this.textures=[];let a=i.count;for(let o=0;o<a;o++)this.textures[o]=s.clone(),this.textures[o].isRenderTargetTexture=!0,this.textures[o].renderTarget=this;this._setTextureOptions(i),this.depthBuffer=i.depthBuffer,this.stencilBuffer=i.stencilBuffer,this.resolveDepthBuffer=i.resolveDepthBuffer,this.resolveStencilBuffer=i.resolveStencilBuffer,this._depthTexture=null,this.depthTexture=i.depthTexture,this.samples=i.samples,this.multiview=i.multiview}_setTextureOptions(e={}){let t={minFilter:On,generateMipmaps:!1,flipY:!1,internalFormat:null};e.mapping!==void 0&&(t.mapping=e.mapping),e.wrapS!==void 0&&(t.wrapS=e.wrapS),e.wrapT!==void 0&&(t.wrapT=e.wrapT),e.wrapR!==void 0&&(t.wrapR=e.wrapR),e.magFilter!==void 0&&(t.magFilter=e.magFilter),e.minFilter!==void 0&&(t.minFilter=e.minFilter),e.format!==void 0&&(t.format=e.format),e.type!==void 0&&(t.type=e.type),e.anisotropy!==void 0&&(t.anisotropy=e.anisotropy),e.colorSpace!==void 0&&(t.colorSpace=e.colorSpace),e.flipY!==void 0&&(t.flipY=e.flipY),e.generateMipmaps!==void 0&&(t.generateMipmaps=e.generateMipmaps),e.internalFormat!==void 0&&(t.internalFormat=e.internalFormat);for(let i=0;i<this.textures.length;i++)this.textures[i].setValues(t)}get texture(){return this.textures[0]}set texture(e){this.textures[0]=e}set depthTexture(e){this._depthTexture!==null&&(this._depthTexture.renderTarget=null),e!==null&&(e.renderTarget=this),this._depthTexture=e}get depthTexture(){return this._depthTexture}setSize(e,t,i=1){if(this.width!==e||this.height!==t||this.depth!==i){this.width=e,this.height=t,this.depth=i;for(let r=0,s=this.textures.length;r<s;r++)this.textures[r].image.width=e,this.textures[r].image.height=t,this.textures[r].image.depth=i,this.textures[r].isArrayTexture=this.textures[r].image.depth>1;this.dispose()}this.viewport.set(0,0,e,t),this.scissor.set(0,0,e,t)}clone(){return new this.constructor().copy(this)}copy(e){this.width=e.width,this.height=e.height,this.depth=e.depth,this.scissor.copy(e.scissor),this.scissorTest=e.scissorTest,this.viewport.copy(e.viewport),this.textures.length=0;for(let t=0,i=e.textures.length;t<i;t++){this.textures[t]=e.textures[t].clone(),this.textures[t].isRenderTargetTexture=!0,this.textures[t].renderTarget=this;let r=Object.assign({},e.textures[t].image);this.textures[t].source=new Xr(r)}return this.depthBuffer=e.depthBuffer,this.stencilBuffer=e.stencilBuffer,this.resolveDepthBuffer=e.resolveDepthBuffer,this.resolveStencilBuffer=e.resolveStencilBuffer,e.depthTexture!==null&&(this.depthTexture=e.depthTexture.clone()),this.samples=e.samples,this}dispose(){this.dispatchEvent({type:"dispose"})}},Wn=class extends go{constructor(e=1,t=1,i={}){super(e,t,i),this.isWebGLRenderTarget=!0}},Fs=class extends on{constructor(e=null,t=1,i=1,r=1){super(null),this.isDataArrayTexture=!0,this.image={data:e,width:t,height:i,depth:r},this.magFilter=sn,this.minFilter=sn,this.wrapR=Ei,this.generateMipmaps=!1,this.flipY=!1,this.unpackAlignment=1,this.layerUpdates=new Set}addLayerUpdate(e){this.layerUpdates.add(e)}clearLayerUpdates(){this.layerUpdates.clear()}};var _o=class extends on{constructor(e=null,t=1,i=1,r=1){super(null),this.isData3DTexture=!0,this.image={data:e,width:t,height:i,depth:r},this.magFilter=sn,this.minFilter=sn,this.wrapR=Ei,this.generateMipmaps=!1,this.flipY=!1,this.unpackAlignment=1}};var qn=class{constructor(e=new L(1/0,1/0,1/0),t=new L(-1/0,-1/0,-1/0)){this.isBox3=!0,this.min=e,this.max=t}set(e,t){return this.min.copy(e),this.max.copy(t),this}setFromArray(e){this.makeEmpty();for(let t=0,i=e.length;t<i;t+=3)this.expandByPoint(Dn.fromArray(e,t));return this}setFromBufferAttribute(e){this.makeEmpty();for(let t=0,i=e.count;t<i;t++)this.expandByPoint(Dn.fromBufferAttribute(e,t));return this}setFromPoints(e){this.makeEmpty();for(let t=0,i=e.length;t<i;t++)this.expandByPoint(e[t]);return this}setFromCenterAndSize(e,t){let i=Dn.copy(t).multiplyScalar(.5);return this.min.copy(e).sub(i),this.max.copy(e).add(i),this}setFromObject(e,t=!1){return this.makeEmpty(),this.expandByObject(e,t)}clone(){return new this.constructor().copy(this)}copy(e){return this.min.copy(e.min),this.max.copy(e.max),this}makeEmpty(){return this.min.x=this.min.y=this.min.z=1/0,this.max.x=this.max.y=this.max.z=-1/0,this}isEmpty(){return this.max.x<this.min.x||this.max.y<this.min.y||this.max.z<this.min.z}getCenter(e){return this.isEmpty()?e.set(0,0,0):e.addVectors(this.min,this.max).multiplyScalar(.5)}getSize(e){return this.isEmpty()?e.set(0,0,0):e.subVectors(this.max,this.min)}expandByPoint(e){return this.min.min(e),this.max.max(e),this}expandByVector(e){return this.min.sub(e),this.max.add(e),this}expandByScalar(e){return this.min.addScalar(-e),this.max.addScalar(e),this}expandByObject(e,t=!1){e.updateWorldMatrix(!1,!1);let i=e.geometry;if(i!==void 0){let s=i.getAttribute("position");if(t===!0&&s!==void 0&&e.isInstancedMesh!==!0)for(let a=0,o=s.count;a<o;a++)e.isMesh===!0?e.getVertexPosition(a,Dn):Dn.fromBufferAttribute(s,a),Dn.applyMatrix4(e.matrixWorld),this.expandByPoint(Dn);else e.boundingBox!==void 0?(e.boundingBox===null&&e.computeBoundingBox(),Ha.copy(e.boundingBox)):(i.boundingBox===null&&i.computeBoundingBox(),Ha.copy(i.boundingBox)),Ha.applyMatrix4(e.matrixWorld),this.union(Ha)}let r=e.children;for(let s=0,a=r.length;s<a;s++)this.expandByObject(r[s],t);return this}containsPoint(e){return e.x>=this.min.x&&e.x<=this.max.x&&e.y>=this.min.y&&e.y<=this.max.y&&e.z>=this.min.z&&e.z<=this.max.z}containsBox(e){return this.min.x<=e.min.x&&e.max.x<=this.max.x&&this.min.y<=e.min.y&&e.max.y<=this.max.y&&this.min.z<=e.min.z&&e.max.z<=this.max.z}getParameter(e,t){return t.set((e.x-this.min.x)/(this.max.x-this.min.x),(e.y-this.min.y)/(this.max.y-this.min.y),(e.z-this.min.z)/(this.max.z-this.min.z))}intersectsBox(e){return e.max.x>=this.min.x&&e.min.x<=this.max.x&&e.max.y>=this.min.y&&e.min.y<=this.max.y&&e.max.z>=this.min.z&&e.min.z<=this.max.z}intersectsSphere(e){return this.clampPoint(e.center,Dn),Dn.distanceToSquared(e.center)<=e.radius*e.radius}intersectsPlane(e){let t,i;return e.normal.x>0?(t=e.normal.x*this.min.x,i=e.normal.x*this.max.x):(t=e.normal.x*this.max.x,i=e.normal.x*this.min.x),e.normal.y>0?(t+=e.normal.y*this.min.y,i+=e.normal.y*this.max.y):(t+=e.normal.y*this.max.y,i+=e.normal.y*this.min.y),e.normal.z>0?(t+=e.normal.z*this.min.z,i+=e.normal.z*this.max.z):(t+=e.normal.z*this.max.z,i+=e.normal.z*this.min.z),t<=-e.constant&&i>=-e.constant}intersectsTriangle(e){if(this.isEmpty())return!1;this.getCenter(ws),Ga.subVectors(this.max,ws),Pr.subVectors(e.a,ws),Ir.subVectors(e.b,ws),Dr.subVectors(e.c,ws),_i.subVectors(Ir,Pr),yi.subVectors(Dr,Ir),Wi.subVectors(Pr,Dr);let t=[0,-_i.z,_i.y,0,-yi.z,yi.y,0,-Wi.z,Wi.y,_i.z,0,-_i.x,yi.z,0,-yi.x,Wi.z,0,-Wi.x,-_i.y,_i.x,0,-yi.y,yi.x,0,-Wi.y,Wi.x,0];return!Pl(t,Pr,Ir,Dr,Ga)||(t=[1,0,0,0,1,0,0,0,1],!Pl(t,Pr,Ir,Dr,Ga))?!1:(Wa.crossVectors(_i,yi),t=[Wa.x,Wa.y,Wa.z],Pl(t,Pr,Ir,Dr,Ga))}clampPoint(e,t){return t.copy(e).clamp(this.min,this.max)}distanceToPoint(e){return this.clampPoint(e,Dn).distanceTo(e)}getBoundingSphere(e){return this.isEmpty()?e.makeEmpty():(this.getCenter(e.center),e.radius=this.getSize(Dn).length()*.5),e}intersect(e){return this.min.max(e.min),this.max.min(e.max),this.isEmpty()&&this.makeEmpty(),this}union(e){return this.min.min(e.min),this.max.max(e.max),this}applyMatrix4(e){return this.isEmpty()?this:(ni[0].set(this.min.x,this.min.y,this.min.z).applyMatrix4(e),ni[1].set(this.min.x,this.min.y,this.max.z).applyMatrix4(e),ni[2].set(this.min.x,this.max.y,this.min.z).applyMatrix4(e),ni[3].set(this.min.x,this.max.y,this.max.z).applyMatrix4(e),ni[4].set(this.max.x,this.min.y,this.min.z).applyMatrix4(e),ni[5].set(this.max.x,this.min.y,this.max.z).applyMatrix4(e),ni[6].set(this.max.x,this.max.y,this.min.z).applyMatrix4(e),ni[7].set(this.max.x,this.max.y,this.max.z).applyMatrix4(e),this.setFromPoints(ni),this)}translate(e){return this.min.add(e),this.max.add(e),this}equals(e){return e.min.equals(this.min)&&e.max.equals(this.max)}toJSON(){return{min:this.min.toArray(),max:this.max.toArray()}}fromJSON(e){return this.min.fromArray(e.min),this.max.fromArray(e.max),this}},ni=[new L,new L,new L,new L,new L,new L,new L,new L],Dn=new L,Ha=new qn,Pr=new L,Ir=new L,Dr=new L,_i=new L,yi=new L,Wi=new L,ws=new L,Ga=new L,Wa=new L,qi=new L;function Pl(n,e,t,i,r){for(let s=0,a=n.length-3;s<=a;s+=3){qi.fromArray(n,s);let o=r.x*Math.abs(qi.x)+r.y*Math.abs(qi.y)+r.z*Math.abs(qi.z),c=e.dot(qi),u=t.dot(qi),l=i.dot(qi);if(Math.max(-Math.max(c,u,l),Math.min(c,u,l))>o)return!1}return!0}var py=new qn,Ts=new L,Il=new L,Ti=class{constructor(e=new L,t=-1){this.isSphere=!0,this.center=e,this.radius=t}set(e,t){return this.center.copy(e),this.radius=t,this}setFromPoints(e,t){let i=this.center;t!==void 0?i.copy(t):py.setFromPoints(e).getCenter(i);let r=0;for(let s=0,a=e.length;s<a;s++)r=Math.max(r,i.distanceToSquared(e[s]));return this.radius=Math.sqrt(r),this}copy(e){return this.center.copy(e.center),this.radius=e.radius,this}isEmpty(){return this.radius<0}makeEmpty(){return this.center.set(0,0,0),this.radius=-1,this}containsPoint(e){return e.distanceToSquared(this.center)<=this.radius*this.radius}distanceToPoint(e){return e.distanceTo(this.center)-this.radius}intersectsSphere(e){let t=this.radius+e.radius;return e.center.distanceToSquared(this.center)<=t*t}intersectsBox(e){return e.intersectsSphere(this)}intersectsPlane(e){return Math.abs(e.distanceToPoint(this.center))<=this.radius}clampPoint(e,t){let i=this.center.distanceToSquared(e);return t.copy(e),i>this.radius*this.radius&&(t.sub(this.center).normalize(),t.multiplyScalar(this.radius).add(this.center)),t}getBoundingBox(e){return this.isEmpty()?(e.makeEmpty(),e):(e.set(this.center,this.center),e.expandByScalar(this.radius),e)}applyMatrix4(e){return this.center.applyMatrix4(e),this.radius=this.radius*e.getMaxScaleOnAxis(),this}translate(e){return this.center.add(e),this}expandByPoint(e){if(this.isEmpty())return this.center.copy(e),this.radius=0,this;Ts.subVectors(e,this.center);let t=Ts.lengthSq();if(t>this.radius*this.radius){let i=Math.sqrt(t),r=(i-this.radius)*.5;this.center.addScaledVector(Ts,r/i),this.radius+=r}return this}union(e){return e.isEmpty()?this:this.isEmpty()?(this.copy(e),this):(this.center.equals(e.center)===!0?this.radius=Math.max(this.radius,e.radius):(Il.subVectors(e.center,this.center).setLength(e.radius),this.expandByPoint(Ts.copy(e.center).add(Il)),this.expandByPoint(Ts.copy(e.center).sub(Il))),this)}equals(e){return e.center.equals(this.center)&&e.radius===this.radius}clone(){return new this.constructor().copy(this)}toJSON(){return{radius:this.radius,center:this.center.toArray()}}fromJSON(e){return this.radius=e.radius,this.center.fromArray(e.center),this}},ii=new L,Dl=new L,qa=new L,vi=new L,Nl=new L,Xa=new L,Ll=new L,er=class{constructor(e=new L,t=new L(0,0,-1)){this.origin=e,this.direction=t}set(e,t){return this.origin.copy(e),this.direction.copy(t),this}copy(e){return this.origin.copy(e.origin),this.direction.copy(e.direction),this}at(e,t){return t.copy(this.origin).addScaledVector(this.direction,e)}lookAt(e){return this.direction.copy(e).sub(this.origin).normalize(),this}recast(e){return this.origin.copy(this.at(e,ii)),this}closestPointToPoint(e,t){t.subVectors(e,this.origin);let i=t.dot(this.direction);return i<0?t.copy(this.origin):t.copy(this.origin).addScaledVector(this.direction,i)}distanceToPoint(e){return Math.sqrt(this.distanceSqToPoint(e))}distanceSqToPoint(e){let t=ii.subVectors(e,this.origin).dot(this.direction);return t<0?this.origin.distanceToSquared(e):(ii.copy(this.origin).addScaledVector(this.direction,t),ii.distanceToSquared(e))}distanceSqToSegment(e,t,i,r){Dl.copy(e).add(t).multiplyScalar(.5),qa.copy(t).sub(e).normalize(),vi.copy(this.origin).sub(Dl);let s=e.distanceTo(t)*.5,a=-this.direction.dot(qa),o=vi.dot(this.direction),c=-vi.dot(qa),u=vi.lengthSq(),l=Math.abs(1-a*a),h,f,p,g;if(l>0)if(h=a*c-o,f=a*o-c,g=s*l,h>=0)if(f>=-g)if(f<=g){let _=1/l;h*=_,f*=_,p=h*(h+a*f+2*o)+f*(a*h+f+2*c)+u}else f=s,h=Math.max(0,-(a*f+o)),p=-h*h+f*(f+2*c)+u;else f=-s,h=Math.max(0,-(a*f+o)),p=-h*h+f*(f+2*c)+u;else f<=-g?(h=Math.max(0,-(-a*s+o)),f=h>0?-s:Math.min(Math.max(-s,-c),s),p=-h*h+f*(f+2*c)+u):f<=g?(h=0,f=Math.min(Math.max(-s,-c),s),p=f*(f+2*c)+u):(h=Math.max(0,-(a*s+o)),f=h>0?s:Math.min(Math.max(-s,-c),s),p=-h*h+f*(f+2*c)+u);else f=a>0?-s:s,h=Math.max(0,-(a*f+o)),p=-h*h+f*(f+2*c)+u;return i&&i.copy(this.origin).addScaledVector(this.direction,h),r&&r.copy(Dl).addScaledVector(qa,f),p}intersectSphere(e,t){ii.subVectors(e.center,this.origin);let i=ii.dot(this.direction),r=ii.dot(ii)-i*i,s=e.radius*e.radius;if(r>s)return null;let a=Math.sqrt(s-r),o=i-a,c=i+a;return c<0?null:o<0?this.at(c,t):this.at(o,t)}intersectsSphere(e){return e.radius<0?!1:this.distanceSqToPoint(e.center)<=e.radius*e.radius}distanceToPlane(e){let t=e.normal.dot(this.direction);if(t===0)return e.distanceToPoint(this.origin)===0?0:null;let i=-(this.origin.dot(e.normal)+e.constant)/t;return i>=0?i:null}intersectPlane(e,t){let i=this.distanceToPlane(e);return i===null?null:this.at(i,t)}intersectsPlane(e){let t=e.distanceToPoint(this.origin);return t===0||e.normal.dot(this.direction)*t<0}intersectBox(e,t){let i,r,s,a,o,c,u=1/this.direction.x,l=1/this.direction.y,h=1/this.direction.z,f=this.origin;return u>=0?(i=(e.min.x-f.x)*u,r=(e.max.x-f.x)*u):(i=(e.max.x-f.x)*u,r=(e.min.x-f.x)*u),l>=0?(s=(e.min.y-f.y)*l,a=(e.max.y-f.y)*l):(s=(e.max.y-f.y)*l,a=(e.min.y-f.y)*l),i>a||s>r||((s>i||isNaN(i))&&(i=s),(a<r||isNaN(r))&&(r=a),h>=0?(o=(e.min.z-f.z)*h,c=(e.max.z-f.z)*h):(o=(e.max.z-f.z)*h,c=(e.min.z-f.z)*h),i>c||o>r)||((o>i||i!==i)&&(i=o),(c<r||r!==r)&&(r=c),r<0)?null:this.at(i>=0?i:r,t)}intersectsBox(e){return this.intersectBox(e,ii)!==null}intersectTriangle(e,t,i,r,s){Nl.subVectors(t,e),Xa.subVectors(i,e),Ll.crossVectors(Nl,Xa);let a=this.direction.dot(Ll),o;if(a>0){if(r)return null;o=1}else if(a<0)o=-1,a=-a;else return null;vi.subVectors(this.origin,e);let c=o*this.direction.dot(Xa.crossVectors(vi,Xa));if(c<0)return null;let u=o*this.direction.dot(Nl.cross(vi));if(u<0||c+u>a)return null;let l=-o*vi.dot(Ll);return l<0?null:this.at(l/a,s)}applyMatrix4(e){return this.origin.applyMatrix4(e),this.direction.transformDirection(e),this}equals(e){return e.origin.equals(this.origin)&&e.direction.equals(this.direction)}clone(){return new this.constructor().copy(this)}},_t=class n{constructor(e,t,i,r,s,a,o,c,u,l,h,f,p,g,_,m){n.prototype.isMatrix4=!0,this.elements=[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],e!==void 0&&this.set(e,t,i,r,s,a,o,c,u,l,h,f,p,g,_,m)}set(e,t,i,r,s,a,o,c,u,l,h,f,p,g,_,m){let d=this.elements;return d[0]=e,d[4]=t,d[8]=i,d[12]=r,d[1]=s,d[5]=a,d[9]=o,d[13]=c,d[2]=u,d[6]=l,d[10]=h,d[14]=f,d[3]=p,d[7]=g,d[11]=_,d[15]=m,this}identity(){return this.set(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1),this}clone(){return new n().fromArray(this.elements)}copy(e){let t=this.elements,i=e.elements;return t[0]=i[0],t[1]=i[1],t[2]=i[2],t[3]=i[3],t[4]=i[4],t[5]=i[5],t[6]=i[6],t[7]=i[7],t[8]=i[8],t[9]=i[9],t[10]=i[10],t[11]=i[11],t[12]=i[12],t[13]=i[13],t[14]=i[14],t[15]=i[15],this}copyPosition(e){let t=this.elements,i=e.elements;return t[12]=i[12],t[13]=i[13],t[14]=i[14],this}setFromMatrix3(e){let t=e.elements;return this.set(t[0],t[3],t[6],0,t[1],t[4],t[7],0,t[2],t[5],t[8],0,0,0,0,1),this}extractBasis(e,t,i){return e.setFromMatrixColumn(this,0),t.setFromMatrixColumn(this,1),i.setFromMatrixColumn(this,2),this}makeBasis(e,t,i){return this.set(e.x,t.x,i.x,0,e.y,t.y,i.y,0,e.z,t.z,i.z,0,0,0,0,1),this}extractRotation(e){let t=this.elements,i=e.elements,r=1/Nr.setFromMatrixColumn(e,0).length(),s=1/Nr.setFromMatrixColumn(e,1).length(),a=1/Nr.setFromMatrixColumn(e,2).length();return t[0]=i[0]*r,t[1]=i[1]*r,t[2]=i[2]*r,t[3]=0,t[4]=i[4]*s,t[5]=i[5]*s,t[6]=i[6]*s,t[7]=0,t[8]=i[8]*a,t[9]=i[9]*a,t[10]=i[10]*a,t[11]=0,t[12]=0,t[13]=0,t[14]=0,t[15]=1,this}makeRotationFromEuler(e){let t=this.elements,i=e.x,r=e.y,s=e.z,a=Math.cos(i),o=Math.sin(i),c=Math.cos(r),u=Math.sin(r),l=Math.cos(s),h=Math.sin(s);if(e.order==="XYZ"){let f=a*l,p=a*h,g=o*l,_=o*h;t[0]=c*l,t[4]=-c*h,t[8]=u,t[1]=p+g*u,t[5]=f-_*u,t[9]=-o*c,t[2]=_-f*u,t[6]=g+p*u,t[10]=a*c}else if(e.order==="YXZ"){let f=c*l,p=c*h,g=u*l,_=u*h;t[0]=f+_*o,t[4]=g*o-p,t[8]=a*u,t[1]=a*h,t[5]=a*l,t[9]=-o,t[2]=p*o-g,t[6]=_+f*o,t[10]=a*c}else if(e.order==="ZXY"){let f=c*l,p=c*h,g=u*l,_=u*h;t[0]=f-_*o,t[4]=-a*h,t[8]=g+p*o,t[1]=p+g*o,t[5]=a*l,t[9]=_-f*o,t[2]=-a*u,t[6]=o,t[10]=a*c}else if(e.order==="ZYX"){let f=a*l,p=a*h,g=o*l,_=o*h;t[0]=c*l,t[4]=g*u-p,t[8]=f*u+_,t[1]=c*h,t[5]=_*u+f,t[9]=p*u-g,t[2]=-u,t[6]=o*c,t[10]=a*c}else if(e.order==="YZX"){let f=a*c,p=a*u,g=o*c,_=o*u;t[0]=c*l,t[4]=_-f*h,t[8]=g*h+p,t[1]=h,t[5]=a*l,t[9]=-o*l,t[2]=-u*l,t[6]=p*h+g,t[10]=f-_*h}else if(e.order==="XZY"){let f=a*c,p=a*u,g=o*c,_=o*u;t[0]=c*l,t[4]=-h,t[8]=u*l,t[1]=f*h+_,t[5]=a*l,t[9]=p*h-g,t[2]=g*h-p,t[6]=o*l,t[10]=_*h+f}return t[3]=0,t[7]=0,t[11]=0,t[12]=0,t[13]=0,t[14]=0,t[15]=1,this}makeRotationFromQuaternion(e){return this.compose(my,e,gy)}lookAt(e,t,i){let r=this.elements;return _n.subVectors(e,t),_n.lengthSq()===0&&(_n.z=1),_n.normalize(),xi.crossVectors(i,_n),xi.lengthSq()===0&&(Math.abs(i.z)===1?_n.x+=1e-4:_n.z+=1e-4,_n.normalize(),xi.crossVectors(i,_n)),xi.normalize(),ja.crossVectors(_n,xi),r[0]=xi.x,r[4]=ja.x,r[8]=_n.x,r[1]=xi.y,r[5]=ja.y,r[9]=_n.y,r[2]=xi.z,r[6]=ja.z,r[10]=_n.z,this}multiply(e){return this.multiplyMatrices(this,e)}premultiply(e){return this.multiplyMatrices(e,this)}multiplyMatrices(e,t){let i=e.elements,r=t.elements,s=this.elements,a=i[0],o=i[4],c=i[8],u=i[12],l=i[1],h=i[5],f=i[9],p=i[13],g=i[2],_=i[6],m=i[10],d=i[14],w=i[3],M=i[7],x=i[11],T=i[15],R=r[0],P=r[4],O=r[8],b=r[12],S=r[1],I=r[5],k=r[9],V=r[13],W=r[2],j=r[6],G=r[10],ne=r[14],z=r[3],ae=r[7],ue=r[11],we=r[15];return s[0]=a*R+o*S+c*W+u*z,s[4]=a*P+o*I+c*j+u*ae,s[8]=a*O+o*k+c*G+u*ue,s[12]=a*b+o*V+c*ne+u*we,s[1]=l*R+h*S+f*W+p*z,s[5]=l*P+h*I+f*j+p*ae,s[9]=l*O+h*k+f*G+p*ue,s[13]=l*b+h*V+f*ne+p*we,s[2]=g*R+_*S+m*W+d*z,s[6]=g*P+_*I+m*j+d*ae,s[10]=g*O+_*k+m*G+d*ue,s[14]=g*b+_*V+m*ne+d*we,s[3]=w*R+M*S+x*W+T*z,s[7]=w*P+M*I+x*j+T*ae,s[11]=w*O+M*k+x*G+T*ue,s[15]=w*b+M*V+x*ne+T*we,this}multiplyScalar(e){let t=this.elements;return t[0]*=e,t[4]*=e,t[8]*=e,t[12]*=e,t[1]*=e,t[5]*=e,t[9]*=e,t[13]*=e,t[2]*=e,t[6]*=e,t[10]*=e,t[14]*=e,t[3]*=e,t[7]*=e,t[11]*=e,t[15]*=e,this}determinant(){let e=this.elements,t=e[0],i=e[4],r=e[8],s=e[12],a=e[1],o=e[5],c=e[9],u=e[13],l=e[2],h=e[6],f=e[10],p=e[14],g=e[3],_=e[7],m=e[11],d=e[15];return g*(+s*c*h-r*u*h-s*o*f+i*u*f+r*o*p-i*c*p)+_*(+t*c*p-t*u*f+s*a*f-r*a*p+r*u*l-s*c*l)+m*(+t*u*h-t*o*p-s*a*h+i*a*p+s*o*l-i*u*l)+d*(-r*o*l-t*c*h+t*o*f+r*a*h-i*a*f+i*c*l)}transpose(){let e=this.elements,t;return t=e[1],e[1]=e[4],e[4]=t,t=e[2],e[2]=e[8],e[8]=t,t=e[6],e[6]=e[9],e[9]=t,t=e[3],e[3]=e[12],e[12]=t,t=e[7],e[7]=e[13],e[13]=t,t=e[11],e[11]=e[14],e[14]=t,this}setPosition(e,t,i){let r=this.elements;return e.isVector3?(r[12]=e.x,r[13]=e.y,r[14]=e.z):(r[12]=e,r[13]=t,r[14]=i),this}invert(){let e=this.elements,t=e[0],i=e[1],r=e[2],s=e[3],a=e[4],o=e[5],c=e[6],u=e[7],l=e[8],h=e[9],f=e[10],p=e[11],g=e[12],_=e[13],m=e[14],d=e[15],w=h*m*u-_*f*u+_*c*p-o*m*p-h*c*d+o*f*d,M=g*f*u-l*m*u-g*c*p+a*m*p+l*c*d-a*f*d,x=l*_*u-g*h*u+g*o*p-a*_*p-l*o*d+a*h*d,T=g*h*c-l*_*c-g*o*f+a*_*f+l*o*m-a*h*m,R=t*w+i*M+r*x+s*T;if(R===0)return this.set(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);let P=1/R;return e[0]=w*P,e[1]=(_*f*s-h*m*s-_*r*p+i*m*p+h*r*d-i*f*d)*P,e[2]=(o*m*s-_*c*s+_*r*u-i*m*u-o*r*d+i*c*d)*P,e[3]=(h*c*s-o*f*s-h*r*u+i*f*u+o*r*p-i*c*p)*P,e[4]=M*P,e[5]=(l*m*s-g*f*s+g*r*p-t*m*p-l*r*d+t*f*d)*P,e[6]=(g*c*s-a*m*s-g*r*u+t*m*u+a*r*d-t*c*d)*P,e[7]=(a*f*s-l*c*s+l*r*u-t*f*u-a*r*p+t*c*p)*P,e[8]=x*P,e[9]=(g*h*s-l*_*s-g*i*p+t*_*p+l*i*d-t*h*d)*P,e[10]=(a*_*s-g*o*s+g*i*u-t*_*u-a*i*d+t*o*d)*P,e[11]=(l*o*s-a*h*s-l*i*u+t*h*u+a*i*p-t*o*p)*P,e[12]=T*P,e[13]=(l*_*r-g*h*r+g*i*f-t*_*f-l*i*m+t*h*m)*P,e[14]=(g*o*r-a*_*r-g*i*c+t*_*c+a*i*m-t*o*m)*P,e[15]=(a*h*r-l*o*r+l*i*c-t*h*c-a*i*f+t*o*f)*P,this}scale(e){let t=this.elements,i=e.x,r=e.y,s=e.z;return t[0]*=i,t[4]*=r,t[8]*=s,t[1]*=i,t[5]*=r,t[9]*=s,t[2]*=i,t[6]*=r,t[10]*=s,t[3]*=i,t[7]*=r,t[11]*=s,this}getMaxScaleOnAxis(){let e=this.elements,t=e[0]*e[0]+e[1]*e[1]+e[2]*e[2],i=e[4]*e[4]+e[5]*e[5]+e[6]*e[6],r=e[8]*e[8]+e[9]*e[9]+e[10]*e[10];return Math.sqrt(Math.max(t,i,r))}makeTranslation(e,t,i){return e.isVector3?this.set(1,0,0,e.x,0,1,0,e.y,0,0,1,e.z,0,0,0,1):this.set(1,0,0,e,0,1,0,t,0,0,1,i,0,0,0,1),this}makeRotationX(e){let t=Math.cos(e),i=Math.sin(e);return this.set(1,0,0,0,0,t,-i,0,0,i,t,0,0,0,0,1),this}makeRotationY(e){let t=Math.cos(e),i=Math.sin(e);return this.set(t,0,i,0,0,1,0,0,-i,0,t,0,0,0,0,1),this}makeRotationZ(e){let t=Math.cos(e),i=Math.sin(e);return this.set(t,-i,0,0,i,t,0,0,0,0,1,0,0,0,0,1),this}makeRotationAxis(e,t){let i=Math.cos(t),r=Math.sin(t),s=1-i,a=e.x,o=e.y,c=e.z,u=s*a,l=s*o;return this.set(u*a+i,u*o-r*c,u*c+r*o,0,u*o+r*c,l*o+i,l*c-r*a,0,u*c-r*o,l*c+r*a,s*c*c+i,0,0,0,0,1),this}makeScale(e,t,i){return this.set(e,0,0,0,0,t,0,0,0,0,i,0,0,0,0,1),this}makeShear(e,t,i,r,s,a){return this.set(1,i,s,0,e,1,a,0,t,r,1,0,0,0,0,1),this}compose(e,t,i){let r=this.elements,s=t._x,a=t._y,o=t._z,c=t._w,u=s+s,l=a+a,h=o+o,f=s*u,p=s*l,g=s*h,_=a*l,m=a*h,d=o*h,w=c*u,M=c*l,x=c*h,T=i.x,R=i.y,P=i.z;return r[0]=(1-(_+d))*T,r[1]=(p+x)*T,r[2]=(g-M)*T,r[3]=0,r[4]=(p-x)*R,r[5]=(1-(f+d))*R,r[6]=(m+w)*R,r[7]=0,r[8]=(g+M)*P,r[9]=(m-w)*P,r[10]=(1-(f+_))*P,r[11]=0,r[12]=e.x,r[13]=e.y,r[14]=e.z,r[15]=1,this}decompose(e,t,i){let r=this.elements,s=Nr.set(r[0],r[1],r[2]).length(),a=Nr.set(r[4],r[5],r[6]).length(),o=Nr.set(r[8],r[9],r[10]).length();this.determinant()<0&&(s=-s),e.x=r[12],e.y=r[13],e.z=r[14],Nn.copy(this);let u=1/s,l=1/a,h=1/o;return Nn.elements[0]*=u,Nn.elements[1]*=u,Nn.elements[2]*=u,Nn.elements[4]*=l,Nn.elements[5]*=l,Nn.elements[6]*=l,Nn.elements[8]*=h,Nn.elements[9]*=h,Nn.elements[10]*=h,t.setFromRotationMatrix(Nn),i.x=s,i.y=a,i.z=o,this}makePerspective(e,t,i,r,s,a,o=Un,c=!1){let u=this.elements,l=2*s/(t-e),h=2*s/(i-r),f=(t+e)/(t-e),p=(i+r)/(i-r),g,_;if(c)g=s/(a-s),_=a*s/(a-s);else if(o===Un)g=-(a+s)/(a-s),_=-2*a*s/(a-s);else if(o===Us)g=-a/(a-s),_=-a*s/(a-s);else throw new Error("THREE.Matrix4.makePerspective(): Invalid coordinate system: "+o);return u[0]=l,u[4]=0,u[8]=f,u[12]=0,u[1]=0,u[5]=h,u[9]=p,u[13]=0,u[2]=0,u[6]=0,u[10]=g,u[14]=_,u[3]=0,u[7]=0,u[11]=-1,u[15]=0,this}makeOrthographic(e,t,i,r,s,a,o=Un,c=!1){let u=this.elements,l=2/(t-e),h=2/(i-r),f=-(t+e)/(t-e),p=-(i+r)/(i-r),g,_;if(c)g=1/(a-s),_=a/(a-s);else if(o===Un)g=-2/(a-s),_=-(a+s)/(a-s);else if(o===Us)g=-1/(a-s),_=-s/(a-s);else throw new Error("THREE.Matrix4.makeOrthographic(): Invalid coordinate system: "+o);return u[0]=l,u[4]=0,u[8]=0,u[12]=f,u[1]=0,u[5]=h,u[9]=0,u[13]=p,u[2]=0,u[6]=0,u[10]=g,u[14]=_,u[3]=0,u[7]=0,u[11]=0,u[15]=1,this}equals(e){let t=this.elements,i=e.elements;for(let r=0;r<16;r++)if(t[r]!==i[r])return!1;return!0}fromArray(e,t=0){for(let i=0;i<16;i++)this.elements[i]=e[i+t];return this}toArray(e=[],t=0){let i=this.elements;return e[t]=i[0],e[t+1]=i[1],e[t+2]=i[2],e[t+3]=i[3],e[t+4]=i[4],e[t+5]=i[5],e[t+6]=i[6],e[t+7]=i[7],e[t+8]=i[8],e[t+9]=i[9],e[t+10]=i[10],e[t+11]=i[11],e[t+12]=i[12],e[t+13]=i[13],e[t+14]=i[14],e[t+15]=i[15],e}},Nr=new L,Nn=new _t,my=new L(0,0,0),gy=new L(1,1,1),xi=new L,ja=new L,_n=new L,pf=new _t,mf=new an,Xn=class n{constructor(e=0,t=0,i=0,r=n.DEFAULT_ORDER){this.isEuler=!0,this._x=e,this._y=t,this._z=i,this._order=r}get x(){return this._x}set x(e){this._x=e,this._onChangeCallback()}get y(){return this._y}set y(e){this._y=e,this._onChangeCallback()}get z(){return this._z}set z(e){this._z=e,this._onChangeCallback()}get order(){return this._order}set order(e){this._order=e,this._onChangeCallback()}set(e,t,i,r=this._order){return this._x=e,this._y=t,this._z=i,this._order=r,this._onChangeCallback(),this}clone(){return new this.constructor(this._x,this._y,this._z,this._order)}copy(e){return this._x=e._x,this._y=e._y,this._z=e._z,this._order=e._order,this._onChangeCallback(),this}setFromRotationMatrix(e,t=this._order,i=!0){let r=e.elements,s=r[0],a=r[4],o=r[8],c=r[1],u=r[5],l=r[9],h=r[2],f=r[6],p=r[10];switch(t){case"XYZ":this._y=Math.asin(We(o,-1,1)),Math.abs(o)<.9999999?(this._x=Math.atan2(-l,p),this._z=Math.atan2(-a,s)):(this._x=Math.atan2(f,u),this._z=0);break;case"YXZ":this._x=Math.asin(-We(l,-1,1)),Math.abs(l)<.9999999?(this._y=Math.atan2(o,p),this._z=Math.atan2(c,u)):(this._y=Math.atan2(-h,s),this._z=0);break;case"ZXY":this._x=Math.asin(We(f,-1,1)),Math.abs(f)<.9999999?(this._y=Math.atan2(-h,p),this._z=Math.atan2(-a,u)):(this._y=0,this._z=Math.atan2(c,s));break;case"ZYX":this._y=Math.asin(-We(h,-1,1)),Math.abs(h)<.9999999?(this._x=Math.atan2(f,p),this._z=Math.atan2(c,s)):(this._x=0,this._z=Math.atan2(-a,u));break;case"YZX":this._z=Math.asin(We(c,-1,1)),Math.abs(c)<.9999999?(this._x=Math.atan2(-l,u),this._y=Math.atan2(-h,s)):(this._x=0,this._y=Math.atan2(o,p));break;case"XZY":this._z=Math.asin(-We(a,-1,1)),Math.abs(a)<.9999999?(this._x=Math.atan2(f,u),this._y=Math.atan2(o,s)):(this._x=Math.atan2(-l,p),this._y=0);break;default:console.warn("THREE.Euler: .setFromRotationMatrix() encountered an unknown order: "+t)}return this._order=t,i===!0&&this._onChangeCallback(),this}setFromQuaternion(e,t,i){return pf.makeRotationFromQuaternion(e),this.setFromRotationMatrix(pf,t,i)}setFromVector3(e,t=this._order){return this.set(e.x,e.y,e.z,t)}reorder(e){return mf.setFromEuler(this),this.setFromQuaternion(mf,e)}equals(e){return e._x===this._x&&e._y===this._y&&e._z===this._z&&e._order===this._order}fromArray(e){return this._x=e[0],this._y=e[1],this._z=e[2],e[3]!==void 0&&(this._order=e[3]),this._onChangeCallback(),this}toArray(e=[],t=0){return e[t]=this._x,e[t+1]=this._y,e[t+2]=this._z,e[t+3]=this._order,e}_onChange(e){return this._onChangeCallback=e,this}_onChangeCallback(){}*[Symbol.iterator](){yield this._x,yield this._y,yield this._z,yield this._order}};Xn.DEFAULT_ORDER="XYZ";var jr=class{constructor(){this.mask=1}set(e){this.mask=(1<<e|0)>>>0}enable(e){this.mask|=1<<e|0}enableAll(){this.mask=-1}toggle(e){this.mask^=1<<e|0}disable(e){this.mask&=~(1<<e|0)}disableAll(){this.mask=0}test(e){return(this.mask&e.mask)!==0}isEnabled(e){return(this.mask&(1<<e|0))!==0}},_y=0,gf=new L,Lr=new an,ri=new _t,Ya=new L,As=new L,yy=new L,vy=new an,_f=new L(1,0,0),yf=new L(0,1,0),vf=new L(0,0,1),xf={type:"added"},xy={type:"removed"},Ur={type:"childadded",child:null},Ul={type:"childremoved",child:null},cn=class n extends Gn{constructor(){super(),this.isObject3D=!0,Object.defineProperty(this,"id",{value:_y++}),this.uuid=ns(),this.name="",this.type="Object3D",this.parent=null,this.children=[],this.up=n.DEFAULT_UP.clone();let e=new L,t=new Xn,i=new an,r=new L(1,1,1);function s(){i.setFromEuler(t,!1)}function a(){t.setFromQuaternion(i,void 0,!1)}t._onChange(s),i._onChange(a),Object.defineProperties(this,{position:{configurable:!0,enumerable:!0,value:e},rotation:{configurable:!0,enumerable:!0,value:t},quaternion:{configurable:!0,enumerable:!0,value:i},scale:{configurable:!0,enumerable:!0,value:r},modelViewMatrix:{value:new _t},normalMatrix:{value:new Fe}}),this.matrix=new _t,this.matrixWorld=new _t,this.matrixAutoUpdate=n.DEFAULT_MATRIX_AUTO_UPDATE,this.matrixWorldAutoUpdate=n.DEFAULT_MATRIX_WORLD_AUTO_UPDATE,this.matrixWorldNeedsUpdate=!1,this.layers=new jr,this.visible=!0,this.castShadow=!1,this.receiveShadow=!1,this.frustumCulled=!0,this.renderOrder=0,this.animations=[],this.customDepthMaterial=void 0,this.customDistanceMaterial=void 0,this.userData={}}onBeforeShadow(){}onAfterShadow(){}onBeforeRender(){}onAfterRender(){}applyMatrix4(e){this.matrixAutoUpdate&&this.updateMatrix(),this.matrix.premultiply(e),this.matrix.decompose(this.position,this.quaternion,this.scale)}applyQuaternion(e){return this.quaternion.premultiply(e),this}setRotationFromAxisAngle(e,t){this.quaternion.setFromAxisAngle(e,t)}setRotationFromEuler(e){this.quaternion.setFromEuler(e,!0)}setRotationFromMatrix(e){this.quaternion.setFromRotationMatrix(e)}setRotationFromQuaternion(e){this.quaternion.copy(e)}rotateOnAxis(e,t){return Lr.setFromAxisAngle(e,t),this.quaternion.multiply(Lr),this}rotateOnWorldAxis(e,t){return Lr.setFromAxisAngle(e,t),this.quaternion.premultiply(Lr),this}rotateX(e){return this.rotateOnAxis(_f,e)}rotateY(e){return this.rotateOnAxis(yf,e)}rotateZ(e){return this.rotateOnAxis(vf,e)}translateOnAxis(e,t){return gf.copy(e).applyQuaternion(this.quaternion),this.position.add(gf.multiplyScalar(t)),this}translateX(e){return this.translateOnAxis(_f,e)}translateY(e){return this.translateOnAxis(yf,e)}translateZ(e){return this.translateOnAxis(vf,e)}localToWorld(e){return this.updateWorldMatrix(!0,!1),e.applyMatrix4(this.matrixWorld)}worldToLocal(e){return this.updateWorldMatrix(!0,!1),e.applyMatrix4(ri.copy(this.matrixWorld).invert())}lookAt(e,t,i){e.isVector3?Ya.copy(e):Ya.set(e,t,i);let r=this.parent;this.updateWorldMatrix(!0,!1),As.setFromMatrixPosition(this.matrixWorld),this.isCamera||this.isLight?ri.lookAt(As,Ya,this.up):ri.lookAt(Ya,As,this.up),this.quaternion.setFromRotationMatrix(ri),r&&(ri.extractRotation(r.matrixWorld),Lr.setFromRotationMatrix(ri),this.quaternion.premultiply(Lr.invert()))}add(e){if(arguments.length>1){for(let t=0;t<arguments.length;t++)this.add(arguments[t]);return this}return e===this?(console.error("THREE.Object3D.add: object can't be added as a child of itself.",e),this):(e&&e.isObject3D?(e.removeFromParent(),e.parent=this,this.children.push(e),e.dispatchEvent(xf),Ur.child=e,this.dispatchEvent(Ur),Ur.child=null):console.error("THREE.Object3D.add: object not an instance of THREE.Object3D.",e),this)}remove(e){if(arguments.length>1){for(let i=0;i<arguments.length;i++)this.remove(arguments[i]);return this}let t=this.children.indexOf(e);return t!==-1&&(e.parent=null,this.children.splice(t,1),e.dispatchEvent(xy),Ul.child=e,this.dispatchEvent(Ul),Ul.child=null),this}removeFromParent(){let e=this.parent;return e!==null&&e.remove(this),this}clear(){return this.remove(...this.children)}attach(e){return this.updateWorldMatrix(!0,!1),ri.copy(this.matrixWorld).invert(),e.parent!==null&&(e.parent.updateWorldMatrix(!0,!1),ri.multiply(e.parent.matrixWorld)),e.applyMatrix4(ri),e.removeFromParent(),e.parent=this,this.children.push(e),e.updateWorldMatrix(!1,!0),e.dispatchEvent(xf),Ur.child=e,this.dispatchEvent(Ur),Ur.child=null,this}getObjectById(e){return this.getObjectByProperty("id",e)}getObjectByName(e){return this.getObjectByProperty("name",e)}getObjectByProperty(e,t){if(this[e]===t)return this;for(let i=0,r=this.children.length;i<r;i++){let a=this.children[i].getObjectByProperty(e,t);if(a!==void 0)return a}}getObjectsByProperty(e,t,i=[]){this[e]===t&&i.push(this);let r=this.children;for(let s=0,a=r.length;s<a;s++)r[s].getObjectsByProperty(e,t,i);return i}getWorldPosition(e){return this.updateWorldMatrix(!0,!1),e.setFromMatrixPosition(this.matrixWorld)}getWorldQuaternion(e){return this.updateWorldMatrix(!0,!1),this.matrixWorld.decompose(As,e,yy),e}getWorldScale(e){return this.updateWorldMatrix(!0,!1),this.matrixWorld.decompose(As,vy,e),e}getWorldDirection(e){this.updateWorldMatrix(!0,!1);let t=this.matrixWorld.elements;return e.set(t[8],t[9],t[10]).normalize()}raycast(){}traverse(e){e(this);let t=this.children;for(let i=0,r=t.length;i<r;i++)t[i].traverse(e)}traverseVisible(e){if(this.visible===!1)return;e(this);let t=this.children;for(let i=0,r=t.length;i<r;i++)t[i].traverseVisible(e)}traverseAncestors(e){let t=this.parent;t!==null&&(e(t),t.traverseAncestors(e))}updateMatrix(){this.matrix.compose(this.position,this.quaternion,this.scale),this.matrixWorldNeedsUpdate=!0}updateMatrixWorld(e){this.matrixAutoUpdate&&this.updateMatrix(),(this.matrixWorldNeedsUpdate||e)&&(this.matrixWorldAutoUpdate===!0&&(this.parent===null?this.matrixWorld.copy(this.matrix):this.matrixWorld.multiplyMatrices(this.parent.matrixWorld,this.matrix)),this.matrixWorldNeedsUpdate=!1,e=!0);let t=this.children;for(let i=0,r=t.length;i<r;i++)t[i].updateMatrixWorld(e)}updateWorldMatrix(e,t){let i=this.parent;if(e===!0&&i!==null&&i.updateWorldMatrix(!0,!1),this.matrixAutoUpdate&&this.updateMatrix(),this.matrixWorldAutoUpdate===!0&&(this.parent===null?this.matrixWorld.copy(this.matrix):this.matrixWorld.multiplyMatrices(this.parent.matrixWorld,this.matrix)),t===!0){let r=this.children;for(let s=0,a=r.length;s<a;s++)r[s].updateWorldMatrix(!1,!0)}}toJSON(e){let t=e===void 0||typeof e=="string",i={};t&&(e={geometries:{},materials:{},textures:{},images:{},shapes:{},skeletons:{},animations:{},nodes:{}},i.metadata={version:4.7,type:"Object",generator:"Object3D.toJSON"});let r={};r.uuid=this.uuid,r.type=this.type,this.name!==""&&(r.name=this.name),this.castShadow===!0&&(r.castShadow=!0),this.receiveShadow===!0&&(r.receiveShadow=!0),this.visible===!1&&(r.visible=!1),this.frustumCulled===!1&&(r.frustumCulled=!1),this.renderOrder!==0&&(r.renderOrder=this.renderOrder),Object.keys(this.userData).length>0&&(r.userData=this.userData),r.layers=this.layers.mask,r.matrix=this.matrix.toArray(),r.up=this.up.toArray(),this.matrixAutoUpdate===!1&&(r.matrixAutoUpdate=!1),this.isInstancedMesh&&(r.type="InstancedMesh",r.count=this.count,r.instanceMatrix=this.instanceMatrix.toJSON(),this.instanceColor!==null&&(r.instanceColor=this.instanceColor.toJSON())),this.isBatchedMesh&&(r.type="BatchedMesh",r.perObjectFrustumCulled=this.perObjectFrustumCulled,r.sortObjects=this.sortObjects,r.drawRanges=this._drawRanges,r.reservedRanges=this._reservedRanges,r.geometryInfo=this._geometryInfo.map(o=>({...o,boundingBox:o.boundingBox?o.boundingBox.toJSON():void 0,boundingSphere:o.boundingSphere?o.boundingSphere.toJSON():void 0})),r.instanceInfo=this._instanceInfo.map(o=>({...o})),r.availableInstanceIds=this._availableInstanceIds.slice(),r.availableGeometryIds=this._availableGeometryIds.slice(),r.nextIndexStart=this._nextIndexStart,r.nextVertexStart=this._nextVertexStart,r.geometryCount=this._geometryCount,r.maxInstanceCount=this._maxInstanceCount,r.maxVertexCount=this._maxVertexCount,r.maxIndexCount=this._maxIndexCount,r.geometryInitialized=this._geometryInitialized,r.matricesTexture=this._matricesTexture.toJSON(e),r.indirectTexture=this._indirectTexture.toJSON(e),this._colorsTexture!==null&&(r.colorsTexture=this._colorsTexture.toJSON(e)),this.boundingSphere!==null&&(r.boundingSphere=this.boundingSphere.toJSON()),this.boundingBox!==null&&(r.boundingBox=this.boundingBox.toJSON()));function s(o,c){return o[c.uuid]===void 0&&(o[c.uuid]=c.toJSON(e)),c.uuid}if(this.isScene)this.background&&(this.background.isColor?r.background=this.background.toJSON():this.background.isTexture&&(r.background=this.background.toJSON(e).uuid)),this.environment&&this.environment.isTexture&&this.environment.isRenderTargetTexture!==!0&&(r.environment=this.environment.toJSON(e).uuid);else if(this.isMesh||this.isLine||this.isPoints){r.geometry=s(e.geometries,this.geometry);let o=this.geometry.parameters;if(o!==void 0&&o.shapes!==void 0){let c=o.shapes;if(Array.isArray(c))for(let u=0,l=c.length;u<l;u++){let h=c[u];s(e.shapes,h)}else s(e.shapes,c)}}if(this.isSkinnedMesh&&(r.bindMode=this.bindMode,r.bindMatrix=this.bindMatrix.toArray(),this.skeleton!==void 0&&(s(e.skeletons,this.skeleton),r.skeleton=this.skeleton.uuid)),this.material!==void 0)if(Array.isArray(this.material)){let o=[];for(let c=0,u=this.material.length;c<u;c++)o.push(s(e.materials,this.material[c]));r.material=o}else r.material=s(e.materials,this.material);if(this.children.length>0){r.children=[];for(let o=0;o<this.children.length;o++)r.children.push(this.children[o].toJSON(e).object)}if(this.animations.length>0){r.animations=[];for(let o=0;o<this.animations.length;o++){let c=this.animations[o];r.animations.push(s(e.animations,c))}}if(t){let o=a(e.geometries),c=a(e.materials),u=a(e.textures),l=a(e.images),h=a(e.shapes),f=a(e.skeletons),p=a(e.animations),g=a(e.nodes);o.length>0&&(i.geometries=o),c.length>0&&(i.materials=c),u.length>0&&(i.textures=u),l.length>0&&(i.images=l),h.length>0&&(i.shapes=h),f.length>0&&(i.skeletons=f),p.length>0&&(i.animations=p),g.length>0&&(i.nodes=g)}return i.object=r,i;function a(o){let c=[];for(let u in o){let l=o[u];delete l.metadata,c.push(l)}return c}}clone(e){return new this.constructor().copy(this,e)}copy(e,t=!0){if(this.name=e.name,this.up.copy(e.up),this.position.copy(e.position),this.rotation.order=e.rotation.order,this.quaternion.copy(e.quaternion),this.scale.copy(e.scale),this.matrix.copy(e.matrix),this.matrixWorld.copy(e.matrixWorld),this.matrixAutoUpdate=e.matrixAutoUpdate,this.matrixWorldAutoUpdate=e.matrixWorldAutoUpdate,this.matrixWorldNeedsUpdate=e.matrixWorldNeedsUpdate,this.layers.mask=e.layers.mask,this.visible=e.visible,this.castShadow=e.castShadow,this.receiveShadow=e.receiveShadow,this.frustumCulled=e.frustumCulled,this.renderOrder=e.renderOrder,this.animations=e.animations.slice(),this.userData=JSON.parse(JSON.stringify(e.userData)),t===!0)for(let i=0;i<e.children.length;i++){let r=e.children[i];this.add(r.clone())}return this}};cn.DEFAULT_UP=new L(0,1,0);cn.DEFAULT_MATRIX_AUTO_UPDATE=!0;cn.DEFAULT_MATRIX_WORLD_AUTO_UPDATE=!0;var Ln=new L,si=new L,Ol=new L,ai=new L,Or=new L,Fr=new L,Sf=new L,Fl=new L,Bl=new L,kl=new L,zl=new Tt,Vl=new Tt,$l=new Tt,Mi=class n{constructor(e=new L,t=new L,i=new L){this.a=e,this.b=t,this.c=i}static getNormal(e,t,i,r){r.subVectors(i,t),Ln.subVectors(e,t),r.cross(Ln);let s=r.lengthSq();return s>0?r.multiplyScalar(1/Math.sqrt(s)):r.set(0,0,0)}static getBarycoord(e,t,i,r,s){Ln.subVectors(r,t),si.subVectors(i,t),Ol.subVectors(e,t);let a=Ln.dot(Ln),o=Ln.dot(si),c=Ln.dot(Ol),u=si.dot(si),l=si.dot(Ol),h=a*u-o*o;if(h===0)return s.set(0,0,0),null;let f=1/h,p=(u*c-o*l)*f,g=(a*l-o*c)*f;return s.set(1-p-g,g,p)}static containsPoint(e,t,i,r){return this.getBarycoord(e,t,i,r,ai)===null?!1:ai.x>=0&&ai.y>=0&&ai.x+ai.y<=1}static getInterpolation(e,t,i,r,s,a,o,c){return this.getBarycoord(e,t,i,r,ai)===null?(c.x=0,c.y=0,"z"in c&&(c.z=0),"w"in c&&(c.w=0),null):(c.setScalar(0),c.addScaledVector(s,ai.x),c.addScaledVector(a,ai.y),c.addScaledVector(o,ai.z),c)}static getInterpolatedAttribute(e,t,i,r,s,a){return zl.setScalar(0),Vl.setScalar(0),$l.setScalar(0),zl.fromBufferAttribute(e,t),Vl.fromBufferAttribute(e,i),$l.fromBufferAttribute(e,r),a.setScalar(0),a.addScaledVector(zl,s.x),a.addScaledVector(Vl,s.y),a.addScaledVector($l,s.z),a}static isFrontFacing(e,t,i,r){return Ln.subVectors(i,t),si.subVectors(e,t),Ln.cross(si).dot(r)<0}set(e,t,i){return this.a.copy(e),this.b.copy(t),this.c.copy(i),this}setFromPointsAndIndices(e,t,i,r){return this.a.copy(e[t]),this.b.copy(e[i]),this.c.copy(e[r]),this}setFromAttributeAndIndices(e,t,i,r){return this.a.fromBufferAttribute(e,t),this.b.fromBufferAttribute(e,i),this.c.fromBufferAttribute(e,r),this}clone(){return new this.constructor().copy(this)}copy(e){return this.a.copy(e.a),this.b.copy(e.b),this.c.copy(e.c),this}getArea(){return Ln.subVectors(this.c,this.b),si.subVectors(this.a,this.b),Ln.cross(si).length()*.5}getMidpoint(e){return e.addVectors(this.a,this.b).add(this.c).multiplyScalar(1/3)}getNormal(e){return n.getNormal(this.a,this.b,this.c,e)}getPlane(e){return e.setFromCoplanarPoints(this.a,this.b,this.c)}getBarycoord(e,t){return n.getBarycoord(e,this.a,this.b,this.c,t)}getInterpolation(e,t,i,r,s){return n.getInterpolation(e,this.a,this.b,this.c,t,i,r,s)}containsPoint(e){return n.containsPoint(e,this.a,this.b,this.c)}isFrontFacing(e){return n.isFrontFacing(this.a,this.b,this.c,e)}intersectsBox(e){return e.intersectsTriangle(this)}closestPointToPoint(e,t){let i=this.a,r=this.b,s=this.c,a,o;Or.subVectors(r,i),Fr.subVectors(s,i),Fl.subVectors(e,i);let c=Or.dot(Fl),u=Fr.dot(Fl);if(c<=0&&u<=0)return t.copy(i);Bl.subVectors(e,r);let l=Or.dot(Bl),h=Fr.dot(Bl);if(l>=0&&h<=l)return t.copy(r);let f=c*h-l*u;if(f<=0&&c>=0&&l<=0)return a=c/(c-l),t.copy(i).addScaledVector(Or,a);kl.subVectors(e,s);let p=Or.dot(kl),g=Fr.dot(kl);if(g>=0&&p<=g)return t.copy(s);let _=p*u-c*g;if(_<=0&&u>=0&&g<=0)return o=u/(u-g),t.copy(i).addScaledVector(Fr,o);let m=l*g-p*h;if(m<=0&&h-l>=0&&p-g>=0)return Sf.subVectors(s,r),o=(h-l)/(h-l+(p-g)),t.copy(r).addScaledVector(Sf,o);let d=1/(m+_+f);return a=_*d,o=f*d,t.copy(i).addScaledVector(Or,a).addScaledVector(Fr,o)}equals(e){return e.a.equals(this.a)&&e.b.equals(this.b)&&e.c.equals(this.c)}},Sp={aliceblue:15792383,antiquewhite:16444375,aqua:65535,aquamarine:8388564,azure:15794175,beige:16119260,bisque:16770244,black:0,blanchedalmond:16772045,blue:255,blueviolet:9055202,brown:10824234,burlywood:14596231,cadetblue:6266528,chartreuse:8388352,chocolate:13789470,coral:16744272,cornflowerblue:6591981,cornsilk:16775388,crimson:14423100,cyan:65535,darkblue:139,darkcyan:35723,darkgoldenrod:12092939,darkgray:11119017,darkgreen:25600,darkgrey:11119017,darkkhaki:12433259,darkmagenta:9109643,darkolivegreen:5597999,darkorange:16747520,darkorchid:10040012,darkred:9109504,darksalmon:15308410,darkseagreen:9419919,darkslateblue:4734347,darkslategray:3100495,darkslategrey:3100495,darkturquoise:52945,darkviolet:9699539,deeppink:16716947,deepskyblue:49151,dimgray:6908265,dimgrey:6908265,dodgerblue:2003199,firebrick:11674146,floralwhite:16775920,forestgreen:2263842,fuchsia:16711935,gainsboro:14474460,ghostwhite:16316671,gold:16766720,goldenrod:14329120,gray:8421504,green:32768,greenyellow:11403055,grey:8421504,honeydew:15794160,hotpink:16738740,indianred:13458524,indigo:4915330,ivory:16777200,khaki:15787660,lavender:15132410,lavenderblush:16773365,lawngreen:8190976,lemonchiffon:16775885,lightblue:11393254,lightcoral:15761536,lightcyan:14745599,lightgoldenrodyellow:16448210,lightgray:13882323,lightgreen:9498256,lightgrey:13882323,lightpink:16758465,lightsalmon:16752762,lightseagreen:2142890,lightskyblue:8900346,lightslategray:7833753,lightslategrey:7833753,lightsteelblue:11584734,lightyellow:16777184,lime:65280,limegreen:3329330,linen:16445670,magenta:16711935,maroon:8388608,mediumaquamarine:6737322,mediumblue:205,mediumorchid:12211667,mediumpurple:9662683,mediumseagreen:3978097,mediumslateblue:8087790,mediumspringgreen:64154,mediumturquoise:4772300,mediumvioletred:13047173,midnightblue:1644912,mintcream:16121850,mistyrose:16770273,moccasin:16770229,navajowhite:16768685,navy:128,oldlace:16643558,olive:8421376,olivedrab:7048739,orange:16753920,orangered:16729344,orchid:14315734,palegoldenrod:15657130,palegreen:10025880,paleturquoise:11529966,palevioletred:14381203,papayawhip:16773077,peachpuff:16767673,peru:13468991,pink:16761035,plum:14524637,powderblue:11591910,purple:8388736,rebeccapurple:6697881,red:16711680,rosybrown:12357519,royalblue:4286945,saddlebrown:9127187,salmon:16416882,sandybrown:16032864,seagreen:3050327,seashell:16774638,sienna:10506797,silver:12632256,skyblue:8900331,slateblue:6970061,slategray:7372944,slategrey:7372944,snow:16775930,springgreen:65407,steelblue:4620980,tan:13808780,teal:32896,thistle:14204888,tomato:16737095,turquoise:4251856,violet:15631086,wheat:16113331,white:16777215,whitesmoke:16119285,yellow:16776960,yellowgreen:10145074},Si={h:0,s:0,l:0},Za={h:0,s:0,l:0};function Hl(n,e,t){return t<0&&(t+=1),t>1&&(t-=1),t<1/6?n+(e-n)*6*t:t<1/2?e:t<2/3?n+(e-n)*6*(2/3-t):n}var Qe=class{constructor(e,t,i){return this.isColor=!0,this.r=1,this.g=1,this.b=1,this.set(e,t,i)}set(e,t,i){if(t===void 0&&i===void 0){let r=e;r&&r.isColor?this.copy(r):typeof r=="number"?this.setHex(r):typeof r=="string"&&this.setStyle(r)}else this.setRGB(e,t,i);return this}setScalar(e){return this.r=e,this.g=e,this.b=e,this}setHex(e,t=Vt){return e=Math.floor(e),this.r=(e>>16&255)/255,this.g=(e>>8&255)/255,this.b=(e&255)/255,tt.colorSpaceToWorking(this,t),this}setRGB(e,t,i,r=tt.workingColorSpace){return this.r=e,this.g=t,this.b=i,tt.colorSpaceToWorking(this,r),this}setHSL(e,t,i,r=tt.workingColorSpace){if(e=gu(e,1),t=We(t,0,1),i=We(i,0,1),t===0)this.r=this.g=this.b=i;else{let s=i<=.5?i*(1+t):i+t-i*t,a=2*i-s;this.r=Hl(a,s,e+1/3),this.g=Hl(a,s,e),this.b=Hl(a,s,e-1/3)}return tt.colorSpaceToWorking(this,r),this}setStyle(e,t=Vt){function i(s){s!==void 0&&parseFloat(s)<1&&console.warn("THREE.Color: Alpha component of "+e+" will be ignored.")}let r;if(r=/^(\w+)\(([^\)]*)\)/.exec(e)){let s,a=r[1],o=r[2];switch(a){case"rgb":case"rgba":if(s=/^\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*(\d*\.?\d+)\s*)?$/.exec(o))return i(s[4]),this.setRGB(Math.min(255,parseInt(s[1],10))/255,Math.min(255,parseInt(s[2],10))/255,Math.min(255,parseInt(s[3],10))/255,t);if(s=/^\s*(\d+)\%\s*,\s*(\d+)\%\s*,\s*(\d+)\%\s*(?:,\s*(\d*\.?\d+)\s*)?$/.exec(o))return i(s[4]),this.setRGB(Math.min(100,parseInt(s[1],10))/100,Math.min(100,parseInt(s[2],10))/100,Math.min(100,parseInt(s[3],10))/100,t);break;case"hsl":case"hsla":if(s=/^\s*(\d*\.?\d+)\s*,\s*(\d*\.?\d+)\%\s*,\s*(\d*\.?\d+)\%\s*(?:,\s*(\d*\.?\d+)\s*)?$/.exec(o))return i(s[4]),this.setHSL(parseFloat(s[1])/360,parseFloat(s[2])/100,parseFloat(s[3])/100,t);break;default:console.warn("THREE.Color: Unknown color model "+e)}}else if(r=/^\#([A-Fa-f\d]+)$/.exec(e)){let s=r[1],a=s.length;if(a===3)return this.setRGB(parseInt(s.charAt(0),16)/15,parseInt(s.charAt(1),16)/15,parseInt(s.charAt(2),16)/15,t);if(a===6)return this.setHex(parseInt(s,16),t);console.warn("THREE.Color: Invalid hex color "+e)}else if(e&&e.length>0)return this.setColorName(e,t);return this}setColorName(e,t=Vt){let i=Sp[e.toLowerCase()];return i!==void 0?this.setHex(i,t):console.warn("THREE.Color: Unknown color "+e),this}clone(){return new this.constructor(this.r,this.g,this.b)}copy(e){return this.r=e.r,this.g=e.g,this.b=e.b,this}copySRGBToLinear(e){return this.r=oi(e.r),this.g=oi(e.g),this.b=oi(e.b),this}copyLinearToSRGB(e){return this.r=Hr(e.r),this.g=Hr(e.g),this.b=Hr(e.b),this}convertSRGBToLinear(){return this.copySRGBToLinear(this),this}convertLinearToSRGB(){return this.copyLinearToSRGB(this),this}getHex(e=Vt){return tt.workingToColorSpace(zt.copy(this),e),Math.round(We(zt.r*255,0,255))*65536+Math.round(We(zt.g*255,0,255))*256+Math.round(We(zt.b*255,0,255))}getHexString(e=Vt){return("000000"+this.getHex(e).toString(16)).slice(-6)}getHSL(e,t=tt.workingColorSpace){tt.workingToColorSpace(zt.copy(this),t);let i=zt.r,r=zt.g,s=zt.b,a=Math.max(i,r,s),o=Math.min(i,r,s),c,u,l=(o+a)/2;if(o===a)c=0,u=0;else{let h=a-o;switch(u=l<=.5?h/(a+o):h/(2-a-o),a){case i:c=(r-s)/h+(r<s?6:0);break;case r:c=(s-i)/h+2;break;case s:c=(i-r)/h+4;break}c/=6}return e.h=c,e.s=u,e.l=l,e}getRGB(e,t=tt.workingColorSpace){return tt.workingToColorSpace(zt.copy(this),t),e.r=zt.r,e.g=zt.g,e.b=zt.b,e}getStyle(e=Vt){tt.workingToColorSpace(zt.copy(this),e);let t=zt.r,i=zt.g,r=zt.b;return e!==Vt?`color(${e} ${t.toFixed(3)} ${i.toFixed(3)} ${r.toFixed(3)})`:`rgb(${Math.round(t*255)},${Math.round(i*255)},${Math.round(r*255)})`}offsetHSL(e,t,i){return this.getHSL(Si),this.setHSL(Si.h+e,Si.s+t,Si.l+i)}add(e){return this.r+=e.r,this.g+=e.g,this.b+=e.b,this}addColors(e,t){return this.r=e.r+t.r,this.g=e.g+t.g,this.b=e.b+t.b,this}addScalar(e){return this.r+=e,this.g+=e,this.b+=e,this}sub(e){return this.r=Math.max(0,this.r-e.r),this.g=Math.max(0,this.g-e.g),this.b=Math.max(0,this.b-e.b),this}multiply(e){return this.r*=e.r,this.g*=e.g,this.b*=e.b,this}multiplyScalar(e){return this.r*=e,this.g*=e,this.b*=e,this}lerp(e,t){return this.r+=(e.r-this.r)*t,this.g+=(e.g-this.g)*t,this.b+=(e.b-this.b)*t,this}lerpColors(e,t,i){return this.r=e.r+(t.r-e.r)*i,this.g=e.g+(t.g-e.g)*i,this.b=e.b+(t.b-e.b)*i,this}lerpHSL(e,t){this.getHSL(Si),e.getHSL(Za);let i=Ds(Si.h,Za.h,t),r=Ds(Si.s,Za.s,t),s=Ds(Si.l,Za.l,t);return this.setHSL(i,r,s),this}setFromVector3(e){return this.r=e.x,this.g=e.y,this.b=e.z,this}applyMatrix3(e){let t=this.r,i=this.g,r=this.b,s=e.elements;return this.r=s[0]*t+s[3]*i+s[6]*r,this.g=s[1]*t+s[4]*i+s[7]*r,this.b=s[2]*t+s[5]*i+s[8]*r,this}equals(e){return e.r===this.r&&e.g===this.g&&e.b===this.b}fromArray(e,t=0){return this.r=e[t],this.g=e[t+1],this.b=e[t+2],this}toArray(e=[],t=0){return e[t]=this.r,e[t+1]=this.g,e[t+2]=this.b,e}fromBufferAttribute(e,t){return this.r=e.getX(t),this.g=e.getY(t),this.b=e.getZ(t),this}toJSON(){return this.getHex()}*[Symbol.iterator](){yield this.r,yield this.g,yield this.b}},zt=new Qe;Qe.NAMES=Sp;var Sy=0,tr=class extends Gn{constructor(){super(),this.isMaterial=!0,Object.defineProperty(this,"id",{value:Sy++}),this.uuid=ns(),this.name="",this.type="Material",this.blending=Ki,this.side=ci,this.vertexColors=!1,this.opacity=1,this.transparent=!1,this.alphaHash=!1,this.blendSrc=lo,this.blendDst=uo,this.blendEquation=wi,this.blendSrcAlpha=null,this.blendDstAlpha=null,this.blendEquationAlpha=null,this.blendColor=new Qe(0,0,0),this.blendAlpha=0,this.depthFunc=Ji,this.depthTest=!0,this.depthWrite=!0,this.stencilWriteMask=255,this.stencilFunc=Zl,this.stencilRef=0,this.stencilFuncMask=255,this.stencilFail=Yi,this.stencilZFail=Yi,this.stencilZPass=Yi,this.stencilWrite=!1,this.clippingPlanes=null,this.clipIntersection=!1,this.clipShadows=!1,this.shadowSide=null,this.colorWrite=!0,this.precision=null,this.polygonOffset=!1,this.polygonOffsetFactor=0,this.polygonOffsetUnits=0,this.dithering=!1,this.alphaToCoverage=!1,this.premultipliedAlpha=!1,this.forceSinglePass=!1,this.allowOverride=!0,this.visible=!0,this.toneMapped=!0,this.userData={},this.version=0,this._alphaTest=0}get alphaTest(){return this._alphaTest}set alphaTest(e){this._alphaTest>0!=e>0&&this.version++,this._alphaTest=e}onBeforeRender(){}onBeforeCompile(){}customProgramCacheKey(){return this.onBeforeCompile.toString()}setValues(e){if(e!==void 0)for(let t in e){let i=e[t];if(i===void 0){console.warn(`THREE.Material: parameter '${t}' has value of undefined.`);continue}let r=this[t];if(r===void 0){console.warn(`THREE.Material: '${t}' is not a property of THREE.${this.type}.`);continue}r&&r.isColor?r.set(i):r&&r.isVector3&&i&&i.isVector3?r.copy(i):this[t]=i}}toJSON(e){let t=e===void 0||typeof e=="string";t&&(e={textures:{},images:{}});let i={metadata:{version:4.7,type:"Material",generator:"Material.toJSON"}};i.uuid=this.uuid,i.type=this.type,this.name!==""&&(i.name=this.name),this.color&&this.color.isColor&&(i.color=this.color.getHex()),this.roughness!==void 0&&(i.roughness=this.roughness),this.metalness!==void 0&&(i.metalness=this.metalness),this.sheen!==void 0&&(i.sheen=this.sheen),this.sheenColor&&this.sheenColor.isColor&&(i.sheenColor=this.sheenColor.getHex()),this.sheenRoughness!==void 0&&(i.sheenRoughness=this.sheenRoughness),this.emissive&&this.emissive.isColor&&(i.emissive=this.emissive.getHex()),this.emissiveIntensity!==void 0&&this.emissiveIntensity!==1&&(i.emissiveIntensity=this.emissiveIntensity),this.specular&&this.specular.isColor&&(i.specular=this.specular.getHex()),this.specularIntensity!==void 0&&(i.specularIntensity=this.specularIntensity),this.specularColor&&this.specularColor.isColor&&(i.specularColor=this.specularColor.getHex()),this.shininess!==void 0&&(i.shininess=this.shininess),this.clearcoat!==void 0&&(i.clearcoat=this.clearcoat),this.clearcoatRoughness!==void 0&&(i.clearcoatRoughness=this.clearcoatRoughness),this.clearcoatMap&&this.clearcoatMap.isTexture&&(i.clearcoatMap=this.clearcoatMap.toJSON(e).uuid),this.clearcoatRoughnessMap&&this.clearcoatRoughnessMap.isTexture&&(i.clearcoatRoughnessMap=this.clearcoatRoughnessMap.toJSON(e).uuid),this.clearcoatNormalMap&&this.clearcoatNormalMap.isTexture&&(i.clearcoatNormalMap=this.clearcoatNormalMap.toJSON(e).uuid,i.clearcoatNormalScale=this.clearcoatNormalScale.toArray()),this.sheenColorMap&&this.sheenColorMap.isTexture&&(i.sheenColorMap=this.sheenColorMap.toJSON(e).uuid),this.sheenRoughnessMap&&this.sheenRoughnessMap.isTexture&&(i.sheenRoughnessMap=this.sheenRoughnessMap.toJSON(e).uuid),this.dispersion!==void 0&&(i.dispersion=this.dispersion),this.iridescence!==void 0&&(i.iridescence=this.iridescence),this.iridescenceIOR!==void 0&&(i.iridescenceIOR=this.iridescenceIOR),this.iridescenceThicknessRange!==void 0&&(i.iridescenceThicknessRange=this.iridescenceThicknessRange),this.iridescenceMap&&this.iridescenceMap.isTexture&&(i.iridescenceMap=this.iridescenceMap.toJSON(e).uuid),this.iridescenceThicknessMap&&this.iridescenceThicknessMap.isTexture&&(i.iridescenceThicknessMap=this.iridescenceThicknessMap.toJSON(e).uuid),this.anisotropy!==void 0&&(i.anisotropy=this.anisotropy),this.anisotropyRotation!==void 0&&(i.anisotropyRotation=this.anisotropyRotation),this.anisotropyMap&&this.anisotropyMap.isTexture&&(i.anisotropyMap=this.anisotropyMap.toJSON(e).uuid),this.map&&this.map.isTexture&&(i.map=this.map.toJSON(e).uuid),this.matcap&&this.matcap.isTexture&&(i.matcap=this.matcap.toJSON(e).uuid),this.alphaMap&&this.alphaMap.isTexture&&(i.alphaMap=this.alphaMap.toJSON(e).uuid),this.lightMap&&this.lightMap.isTexture&&(i.lightMap=this.lightMap.toJSON(e).uuid,i.lightMapIntensity=this.lightMapIntensity),this.aoMap&&this.aoMap.isTexture&&(i.aoMap=this.aoMap.toJSON(e).uuid,i.aoMapIntensity=this.aoMapIntensity),this.bumpMap&&this.bumpMap.isTexture&&(i.bumpMap=this.bumpMap.toJSON(e).uuid,i.bumpScale=this.bumpScale),this.normalMap&&this.normalMap.isTexture&&(i.normalMap=this.normalMap.toJSON(e).uuid,i.normalMapType=this.normalMapType,i.normalScale=this.normalScale.toArray()),this.displacementMap&&this.displacementMap.isTexture&&(i.displacementMap=this.displacementMap.toJSON(e).uuid,i.displacementScale=this.displacementScale,i.displacementBias=this.displacementBias),this.roughnessMap&&this.roughnessMap.isTexture&&(i.roughnessMap=this.roughnessMap.toJSON(e).uuid),this.metalnessMap&&this.metalnessMap.isTexture&&(i.metalnessMap=this.metalnessMap.toJSON(e).uuid),this.emissiveMap&&this.emissiveMap.isTexture&&(i.emissiveMap=this.emissiveMap.toJSON(e).uuid),this.specularMap&&this.specularMap.isTexture&&(i.specularMap=this.specularMap.toJSON(e).uuid),this.specularIntensityMap&&this.specularIntensityMap.isTexture&&(i.specularIntensityMap=this.specularIntensityMap.toJSON(e).uuid),this.specularColorMap&&this.specularColorMap.isTexture&&(i.specularColorMap=this.specularColorMap.toJSON(e).uuid),this.envMap&&this.envMap.isTexture&&(i.envMap=this.envMap.toJSON(e).uuid,this.combine!==void 0&&(i.combine=this.combine)),this.envMapRotation!==void 0&&(i.envMapRotation=this.envMapRotation.toArray()),this.envMapIntensity!==void 0&&(i.envMapIntensity=this.envMapIntensity),this.reflectivity!==void 0&&(i.reflectivity=this.reflectivity),this.refractionRatio!==void 0&&(i.refractionRatio=this.refractionRatio),this.gradientMap&&this.gradientMap.isTexture&&(i.gradientMap=this.gradientMap.toJSON(e).uuid),this.transmission!==void 0&&(i.transmission=this.transmission),this.transmissionMap&&this.transmissionMap.isTexture&&(i.transmissionMap=this.transmissionMap.toJSON(e).uuid),this.thickness!==void 0&&(i.thickness=this.thickness),this.thicknessMap&&this.thicknessMap.isTexture&&(i.thicknessMap=this.thicknessMap.toJSON(e).uuid),this.attenuationDistance!==void 0&&this.attenuationDistance!==1/0&&(i.attenuationDistance=this.attenuationDistance),this.attenuationColor!==void 0&&(i.attenuationColor=this.attenuationColor.getHex()),this.size!==void 0&&(i.size=this.size),this.shadowSide!==null&&(i.shadowSide=this.shadowSide),this.sizeAttenuation!==void 0&&(i.sizeAttenuation=this.sizeAttenuation),this.blending!==Ki&&(i.blending=this.blending),this.side!==ci&&(i.side=this.side),this.vertexColors===!0&&(i.vertexColors=!0),this.opacity<1&&(i.opacity=this.opacity),this.transparent===!0&&(i.transparent=!0),this.blendSrc!==lo&&(i.blendSrc=this.blendSrc),this.blendDst!==uo&&(i.blendDst=this.blendDst),this.blendEquation!==wi&&(i.blendEquation=this.blendEquation),this.blendSrcAlpha!==null&&(i.blendSrcAlpha=this.blendSrcAlpha),this.blendDstAlpha!==null&&(i.blendDstAlpha=this.blendDstAlpha),this.blendEquationAlpha!==null&&(i.blendEquationAlpha=this.blendEquationAlpha),this.blendColor&&this.blendColor.isColor&&(i.blendColor=this.blendColor.getHex()),this.blendAlpha!==0&&(i.blendAlpha=this.blendAlpha),this.depthFunc!==Ji&&(i.depthFunc=this.depthFunc),this.depthTest===!1&&(i.depthTest=this.depthTest),this.depthWrite===!1&&(i.depthWrite=this.depthWrite),this.colorWrite===!1&&(i.colorWrite=this.colorWrite),this.stencilWriteMask!==255&&(i.stencilWriteMask=this.stencilWriteMask),this.stencilFunc!==Zl&&(i.stencilFunc=this.stencilFunc),this.stencilRef!==0&&(i.stencilRef=this.stencilRef),this.stencilFuncMask!==255&&(i.stencilFuncMask=this.stencilFuncMask),this.stencilFail!==Yi&&(i.stencilFail=this.stencilFail),this.stencilZFail!==Yi&&(i.stencilZFail=this.stencilZFail),this.stencilZPass!==Yi&&(i.stencilZPass=this.stencilZPass),this.stencilWrite===!0&&(i.stencilWrite=this.stencilWrite),this.rotation!==void 0&&this.rotation!==0&&(i.rotation=this.rotation),this.polygonOffset===!0&&(i.polygonOffset=!0),this.polygonOffsetFactor!==0&&(i.polygonOffsetFactor=this.polygonOffsetFactor),this.polygonOffsetUnits!==0&&(i.polygonOffsetUnits=this.polygonOffsetUnits),this.linewidth!==void 0&&this.linewidth!==1&&(i.linewidth=this.linewidth),this.dashSize!==void 0&&(i.dashSize=this.dashSize),this.gapSize!==void 0&&(i.gapSize=this.gapSize),this.scale!==void 0&&(i.scale=this.scale),this.dithering===!0&&(i.dithering=!0),this.alphaTest>0&&(i.alphaTest=this.alphaTest),this.alphaHash===!0&&(i.alphaHash=!0),this.alphaToCoverage===!0&&(i.alphaToCoverage=!0),this.premultipliedAlpha===!0&&(i.premultipliedAlpha=!0),this.forceSinglePass===!0&&(i.forceSinglePass=!0),this.wireframe===!0&&(i.wireframe=!0),this.wireframeLinewidth>1&&(i.wireframeLinewidth=this.wireframeLinewidth),this.wireframeLinecap!=="round"&&(i.wireframeLinecap=this.wireframeLinecap),this.wireframeLinejoin!=="round"&&(i.wireframeLinejoin=this.wireframeLinejoin),this.flatShading===!0&&(i.flatShading=!0),this.visible===!1&&(i.visible=!1),this.toneMapped===!1&&(i.toneMapped=!1),this.fog===!1&&(i.fog=!1),Object.keys(this.userData).length>0&&(i.userData=this.userData);function r(s){let a=[];for(let o in s){let c=s[o];delete c.metadata,a.push(c)}return a}if(t){let s=r(e.textures),a=r(e.images);s.length>0&&(i.textures=s),a.length>0&&(i.images=a)}return i}clone(){return new this.constructor().copy(this)}copy(e){this.name=e.name,this.blending=e.blending,this.side=e.side,this.vertexColors=e.vertexColors,this.opacity=e.opacity,this.transparent=e.transparent,this.blendSrc=e.blendSrc,this.blendDst=e.blendDst,this.blendEquation=e.blendEquation,this.blendSrcAlpha=e.blendSrcAlpha,this.blendDstAlpha=e.blendDstAlpha,this.blendEquationAlpha=e.blendEquationAlpha,this.blendColor.copy(e.blendColor),this.blendAlpha=e.blendAlpha,this.depthFunc=e.depthFunc,this.depthTest=e.depthTest,this.depthWrite=e.depthWrite,this.stencilWriteMask=e.stencilWriteMask,this.stencilFunc=e.stencilFunc,this.stencilRef=e.stencilRef,this.stencilFuncMask=e.stencilFuncMask,this.stencilFail=e.stencilFail,this.stencilZFail=e.stencilZFail,this.stencilZPass=e.stencilZPass,this.stencilWrite=e.stencilWrite;let t=e.clippingPlanes,i=null;if(t!==null){let r=t.length;i=new Array(r);for(let s=0;s!==r;++s)i[s]=t[s].clone()}return this.clippingPlanes=i,this.clipIntersection=e.clipIntersection,this.clipShadows=e.clipShadows,this.shadowSide=e.shadowSide,this.colorWrite=e.colorWrite,this.precision=e.precision,this.polygonOffset=e.polygonOffset,this.polygonOffsetFactor=e.polygonOffsetFactor,this.polygonOffsetUnits=e.polygonOffsetUnits,this.dithering=e.dithering,this.alphaTest=e.alphaTest,this.alphaHash=e.alphaHash,this.alphaToCoverage=e.alphaToCoverage,this.premultipliedAlpha=e.premultipliedAlpha,this.forceSinglePass=e.forceSinglePass,this.visible=e.visible,this.toneMapped=e.toneMapped,this.userData=JSON.parse(JSON.stringify(e.userData)),this}dispose(){this.dispatchEvent({type:"dispose"})}set needsUpdate(e){e===!0&&this.version++}},Ai=class extends tr{constructor(e){super(),this.isMeshBasicMaterial=!0,this.type="MeshBasicMaterial",this.color=new Qe(16777215),this.map=null,this.lightMap=null,this.lightMapIntensity=1,this.aoMap=null,this.aoMapIntensity=1,this.specularMap=null,this.alphaMap=null,this.envMap=null,this.envMapRotation=new Xn,this.combine=su,this.reflectivity=1,this.refractionRatio=.98,this.wireframe=!1,this.wireframeLinewidth=1,this.wireframeLinecap="round",this.wireframeLinejoin="round",this.fog=!0,this.setValues(e)}copy(e){return super.copy(e),this.color.copy(e.color),this.map=e.map,this.lightMap=e.lightMap,this.lightMapIntensity=e.lightMapIntensity,this.aoMap=e.aoMap,this.aoMapIntensity=e.aoMapIntensity,this.specularMap=e.specularMap,this.alphaMap=e.alphaMap,this.envMap=e.envMap,this.envMapRotation.copy(e.envMapRotation),this.combine=e.combine,this.reflectivity=e.reflectivity,this.refractionRatio=e.refractionRatio,this.wireframe=e.wireframe,this.wireframeLinewidth=e.wireframeLinewidth,this.wireframeLinecap=e.wireframeLinecap,this.wireframeLinejoin=e.wireframeLinejoin,this.fog=e.fog,this}};var Ct=new L,Ka=new Ue,by=0,rn=class{constructor(e,t,i=!1){if(Array.isArray(e))throw new TypeError("THREE.BufferAttribute: array should be a Typed Array.");this.isBufferAttribute=!0,Object.defineProperty(this,"id",{value:by++}),this.name="",this.array=e,this.itemSize=t,this.count=e!==void 0?e.length/t:0,this.normalized=i,this.usage=Kl,this.updateRanges=[],this.gpuType=Bn,this.version=0}onUploadCallback(){}set needsUpdate(e){e===!0&&this.version++}setUsage(e){return this.usage=e,this}addUpdateRange(e,t){this.updateRanges.push({start:e,count:t})}clearUpdateRanges(){this.updateRanges.length=0}copy(e){return this.name=e.name,this.array=new e.array.constructor(e.array),this.itemSize=e.itemSize,this.count=e.count,this.normalized=e.normalized,this.usage=e.usage,this.gpuType=e.gpuType,this}copyAt(e,t,i){e*=this.itemSize,i*=t.itemSize;for(let r=0,s=this.itemSize;r<s;r++)this.array[e+r]=t.array[i+r];return this}copyArray(e){return this.array.set(e),this}applyMatrix3(e){if(this.itemSize===2)for(let t=0,i=this.count;t<i;t++)Ka.fromBufferAttribute(this,t),Ka.applyMatrix3(e),this.setXY(t,Ka.x,Ka.y);else if(this.itemSize===3)for(let t=0,i=this.count;t<i;t++)Ct.fromBufferAttribute(this,t),Ct.applyMatrix3(e),this.setXYZ(t,Ct.x,Ct.y,Ct.z);return this}applyMatrix4(e){for(let t=0,i=this.count;t<i;t++)Ct.fromBufferAttribute(this,t),Ct.applyMatrix4(e),this.setXYZ(t,Ct.x,Ct.y,Ct.z);return this}applyNormalMatrix(e){for(let t=0,i=this.count;t<i;t++)Ct.fromBufferAttribute(this,t),Ct.applyNormalMatrix(e),this.setXYZ(t,Ct.x,Ct.y,Ct.z);return this}transformDirection(e){for(let t=0,i=this.count;t<i;t++)Ct.fromBufferAttribute(this,t),Ct.transformDirection(e),this.setXYZ(t,Ct.x,Ct.y,Ct.z);return this}set(e,t=0){return this.array.set(e,t),this}getComponent(e,t){let i=this.array[e*this.itemSize+t];return this.normalized&&(i=$r(i,this.array)),i}setComponent(e,t,i){return this.normalized&&(i=Kt(i,this.array)),this.array[e*this.itemSize+t]=i,this}getX(e){let t=this.array[e*this.itemSize];return this.normalized&&(t=$r(t,this.array)),t}setX(e,t){return this.normalized&&(t=Kt(t,this.array)),this.array[e*this.itemSize]=t,this}getY(e){let t=this.array[e*this.itemSize+1];return this.normalized&&(t=$r(t,this.array)),t}setY(e,t){return this.normalized&&(t=Kt(t,this.array)),this.array[e*this.itemSize+1]=t,this}getZ(e){let t=this.array[e*this.itemSize+2];return this.normalized&&(t=$r(t,this.array)),t}setZ(e,t){return this.normalized&&(t=Kt(t,this.array)),this.array[e*this.itemSize+2]=t,this}getW(e){let t=this.array[e*this.itemSize+3];return this.normalized&&(t=$r(t,this.array)),t}setW(e,t){return this.normalized&&(t=Kt(t,this.array)),this.array[e*this.itemSize+3]=t,this}setXY(e,t,i){return e*=this.itemSize,this.normalized&&(t=Kt(t,this.array),i=Kt(i,this.array)),this.array[e+0]=t,this.array[e+1]=i,this}setXYZ(e,t,i,r){return e*=this.itemSize,this.normalized&&(t=Kt(t,this.array),i=Kt(i,this.array),r=Kt(r,this.array)),this.array[e+0]=t,this.array[e+1]=i,this.array[e+2]=r,this}setXYZW(e,t,i,r,s){return e*=this.itemSize,this.normalized&&(t=Kt(t,this.array),i=Kt(i,this.array),r=Kt(r,this.array),s=Kt(s,this.array)),this.array[e+0]=t,this.array[e+1]=i,this.array[e+2]=r,this.array[e+3]=s,this}onUpload(e){return this.onUploadCallback=e,this}clone(){return new this.constructor(this.array,this.itemSize).copy(this)}toJSON(){let e={itemSize:this.itemSize,type:this.array.constructor.name,array:Array.from(this.array),normalized:this.normalized};return this.name!==""&&(e.name=this.name),this.usage!==Kl&&(e.usage=this.usage),e}};var Bs=class extends rn{constructor(e,t,i){super(new Uint16Array(e),t,i)}};var ks=class extends rn{constructor(e,t,i){super(new Uint32Array(e),t,i)}};var vn=class extends rn{constructor(e,t,i){super(new Float32Array(e),t,i)}},My=0,En=new _t,Gl=new cn,Br=new L,yn=new qn,Rs=new qn,Lt=new L,li=class n extends Gn{constructor(){super(),this.isBufferGeometry=!0,Object.defineProperty(this,"id",{value:My++}),this.uuid=ns(),this.name="",this.type="BufferGeometry",this.index=null,this.indirect=null,this.attributes={},this.morphAttributes={},this.morphTargetsRelative=!1,this.groups=[],this.boundingBox=null,this.boundingSphere=null,this.drawRange={start:0,count:1/0},this.userData={}}getIndex(){return this.index}setIndex(e){return Array.isArray(e)?this.index=new(_u(e)?ks:Bs)(e,1):this.index=e,this}setIndirect(e){return this.indirect=e,this}getIndirect(){return this.indirect}getAttribute(e){return this.attributes[e]}setAttribute(e,t){return this.attributes[e]=t,this}deleteAttribute(e){return delete this.attributes[e],this}hasAttribute(e){return this.attributes[e]!==void 0}addGroup(e,t,i=0){this.groups.push({start:e,count:t,materialIndex:i})}clearGroups(){this.groups=[]}setDrawRange(e,t){this.drawRange.start=e,this.drawRange.count=t}applyMatrix4(e){let t=this.attributes.position;t!==void 0&&(t.applyMatrix4(e),t.needsUpdate=!0);let i=this.attributes.normal;if(i!==void 0){let s=new Fe().getNormalMatrix(e);i.applyNormalMatrix(s),i.needsUpdate=!0}let r=this.attributes.tangent;return r!==void 0&&(r.transformDirection(e),r.needsUpdate=!0),this.boundingBox!==null&&this.computeBoundingBox(),this.boundingSphere!==null&&this.computeBoundingSphere(),this}applyQuaternion(e){return En.makeRotationFromQuaternion(e),this.applyMatrix4(En),this}rotateX(e){return En.makeRotationX(e),this.applyMatrix4(En),this}rotateY(e){return En.makeRotationY(e),this.applyMatrix4(En),this}rotateZ(e){return En.makeRotationZ(e),this.applyMatrix4(En),this}translate(e,t,i){return En.makeTranslation(e,t,i),this.applyMatrix4(En),this}scale(e,t,i){return En.makeScale(e,t,i),this.applyMatrix4(En),this}lookAt(e){return Gl.lookAt(e),Gl.updateMatrix(),this.applyMatrix4(Gl.matrix),this}center(){return this.computeBoundingBox(),this.boundingBox.getCenter(Br).negate(),this.translate(Br.x,Br.y,Br.z),this}setFromPoints(e){let t=this.getAttribute("position");if(t===void 0){let i=[];for(let r=0,s=e.length;r<s;r++){let a=e[r];i.push(a.x,a.y,a.z||0)}this.setAttribute("position",new vn(i,3))}else{let i=Math.min(e.length,t.count);for(let r=0;r<i;r++){let s=e[r];t.setXYZ(r,s.x,s.y,s.z||0)}e.length>t.count&&console.warn("THREE.BufferGeometry: Buffer size too small for points data. Use .dispose() and create a new geometry."),t.needsUpdate=!0}return this}computeBoundingBox(){this.boundingBox===null&&(this.boundingBox=new qn);let e=this.attributes.position,t=this.morphAttributes.position;if(e&&e.isGLBufferAttribute){console.error("THREE.BufferGeometry.computeBoundingBox(): GLBufferAttribute requires a manual bounding box.",this),this.boundingBox.set(new L(-1/0,-1/0,-1/0),new L(1/0,1/0,1/0));return}if(e!==void 0){if(this.boundingBox.setFromBufferAttribute(e),t)for(let i=0,r=t.length;i<r;i++){let s=t[i];yn.setFromBufferAttribute(s),this.morphTargetsRelative?(Lt.addVectors(this.boundingBox.min,yn.min),this.boundingBox.expandByPoint(Lt),Lt.addVectors(this.boundingBox.max,yn.max),this.boundingBox.expandByPoint(Lt)):(this.boundingBox.expandByPoint(yn.min),this.boundingBox.expandByPoint(yn.max))}}else this.boundingBox.makeEmpty();(isNaN(this.boundingBox.min.x)||isNaN(this.boundingBox.min.y)||isNaN(this.boundingBox.min.z))&&console.error('THREE.BufferGeometry.computeBoundingBox(): Computed min/max have NaN values. The "position" attribute is likely to have NaN values.',this)}computeBoundingSphere(){this.boundingSphere===null&&(this.boundingSphere=new Ti);let e=this.attributes.position,t=this.morphAttributes.position;if(e&&e.isGLBufferAttribute){console.error("THREE.BufferGeometry.computeBoundingSphere(): GLBufferAttribute requires a manual bounding sphere.",this),this.boundingSphere.set(new L,1/0);return}if(e){let i=this.boundingSphere.center;if(yn.setFromBufferAttribute(e),t)for(let s=0,a=t.length;s<a;s++){let o=t[s];Rs.setFromBufferAttribute(o),this.morphTargetsRelative?(Lt.addVectors(yn.min,Rs.min),yn.expandByPoint(Lt),Lt.addVectors(yn.max,Rs.max),yn.expandByPoint(Lt)):(yn.expandByPoint(Rs.min),yn.expandByPoint(Rs.max))}yn.getCenter(i);let r=0;for(let s=0,a=e.count;s<a;s++)Lt.fromBufferAttribute(e,s),r=Math.max(r,i.distanceToSquared(Lt));if(t)for(let s=0,a=t.length;s<a;s++){let o=t[s],c=this.morphTargetsRelative;for(let u=0,l=o.count;u<l;u++)Lt.fromBufferAttribute(o,u),c&&(Br.fromBufferAttribute(e,u),Lt.add(Br)),r=Math.max(r,i.distanceToSquared(Lt))}this.boundingSphere.radius=Math.sqrt(r),isNaN(this.boundingSphere.radius)&&console.error('THREE.BufferGeometry.computeBoundingSphere(): Computed radius is NaN. The "position" attribute is likely to have NaN values.',this)}}computeTangents(){let e=this.index,t=this.attributes;if(e===null||t.position===void 0||t.normal===void 0||t.uv===void 0){console.error("THREE.BufferGeometry: .computeTangents() failed. Missing required attributes (index, position, normal or uv)");return}let i=t.position,r=t.normal,s=t.uv;this.hasAttribute("tangent")===!1&&this.setAttribute("tangent",new rn(new Float32Array(4*i.count),4));let a=this.getAttribute("tangent"),o=[],c=[];for(let O=0;O<i.count;O++)o[O]=new L,c[O]=new L;let u=new L,l=new L,h=new L,f=new Ue,p=new Ue,g=new Ue,_=new L,m=new L;function d(O,b,S){u.fromBufferAttribute(i,O),l.fromBufferAttribute(i,b),h.fromBufferAttribute(i,S),f.fromBufferAttribute(s,O),p.fromBufferAttribute(s,b),g.fromBufferAttribute(s,S),l.sub(u),h.sub(u),p.sub(f),g.sub(f);let I=1/(p.x*g.y-g.x*p.y);isFinite(I)&&(_.copy(l).multiplyScalar(g.y).addScaledVector(h,-p.y).multiplyScalar(I),m.copy(h).multiplyScalar(p.x).addScaledVector(l,-g.x).multiplyScalar(I),o[O].add(_),o[b].add(_),o[S].add(_),c[O].add(m),c[b].add(m),c[S].add(m))}let w=this.groups;w.length===0&&(w=[{start:0,count:e.count}]);for(let O=0,b=w.length;O<b;++O){let S=w[O],I=S.start,k=S.count;for(let V=I,W=I+k;V<W;V+=3)d(e.getX(V+0),e.getX(V+1),e.getX(V+2))}let M=new L,x=new L,T=new L,R=new L;function P(O){T.fromBufferAttribute(r,O),R.copy(T);let b=o[O];M.copy(b),M.sub(T.multiplyScalar(T.dot(b))).normalize(),x.crossVectors(R,b);let I=x.dot(c[O])<0?-1:1;a.setXYZW(O,M.x,M.y,M.z,I)}for(let O=0,b=w.length;O<b;++O){let S=w[O],I=S.start,k=S.count;for(let V=I,W=I+k;V<W;V+=3)P(e.getX(V+0)),P(e.getX(V+1)),P(e.getX(V+2))}}computeVertexNormals(){let e=this.index,t=this.getAttribute("position");if(t!==void 0){let i=this.getAttribute("normal");if(i===void 0)i=new rn(new Float32Array(t.count*3),3),this.setAttribute("normal",i);else for(let f=0,p=i.count;f<p;f++)i.setXYZ(f,0,0,0);let r=new L,s=new L,a=new L,o=new L,c=new L,u=new L,l=new L,h=new L;if(e)for(let f=0,p=e.count;f<p;f+=3){let g=e.getX(f+0),_=e.getX(f+1),m=e.getX(f+2);r.fromBufferAttribute(t,g),s.fromBufferAttribute(t,_),a.fromBufferAttribute(t,m),l.subVectors(a,s),h.subVectors(r,s),l.cross(h),o.fromBufferAttribute(i,g),c.fromBufferAttribute(i,_),u.fromBufferAttribute(i,m),o.add(l),c.add(l),u.add(l),i.setXYZ(g,o.x,o.y,o.z),i.setXYZ(_,c.x,c.y,c.z),i.setXYZ(m,u.x,u.y,u.z)}else for(let f=0,p=t.count;f<p;f+=3)r.fromBufferAttribute(t,f+0),s.fromBufferAttribute(t,f+1),a.fromBufferAttribute(t,f+2),l.subVectors(a,s),h.subVectors(r,s),l.cross(h),i.setXYZ(f+0,l.x,l.y,l.z),i.setXYZ(f+1,l.x,l.y,l.z),i.setXYZ(f+2,l.x,l.y,l.z);this.normalizeNormals(),i.needsUpdate=!0}}normalizeNormals(){let e=this.attributes.normal;for(let t=0,i=e.count;t<i;t++)Lt.fromBufferAttribute(e,t),Lt.normalize(),e.setXYZ(t,Lt.x,Lt.y,Lt.z)}toNonIndexed(){function e(o,c){let u=o.array,l=o.itemSize,h=o.normalized,f=new u.constructor(c.length*l),p=0,g=0;for(let _=0,m=c.length;_<m;_++){o.isInterleavedBufferAttribute?p=c[_]*o.data.stride+o.offset:p=c[_]*l;for(let d=0;d<l;d++)f[g++]=u[p++]}return new rn(f,l,h)}if(this.index===null)return console.warn("THREE.BufferGeometry.toNonIndexed(): BufferGeometry is already non-indexed."),this;let t=new n,i=this.index.array,r=this.attributes;for(let o in r){let c=r[o],u=e(c,i);t.setAttribute(o,u)}let s=this.morphAttributes;for(let o in s){let c=[],u=s[o];for(let l=0,h=u.length;l<h;l++){let f=u[l],p=e(f,i);c.push(p)}t.morphAttributes[o]=c}t.morphTargetsRelative=this.morphTargetsRelative;let a=this.groups;for(let o=0,c=a.length;o<c;o++){let u=a[o];t.addGroup(u.start,u.count,u.materialIndex)}return t}toJSON(){let e={metadata:{version:4.7,type:"BufferGeometry",generator:"BufferGeometry.toJSON"}};if(e.uuid=this.uuid,e.type=this.type,this.name!==""&&(e.name=this.name),Object.keys(this.userData).length>0&&(e.userData=this.userData),this.parameters!==void 0){let c=this.parameters;for(let u in c)c[u]!==void 0&&(e[u]=c[u]);return e}e.data={attributes:{}};let t=this.index;t!==null&&(e.data.index={type:t.array.constructor.name,array:Array.prototype.slice.call(t.array)});let i=this.attributes;for(let c in i){let u=i[c];e.data.attributes[c]=u.toJSON(e.data)}let r={},s=!1;for(let c in this.morphAttributes){let u=this.morphAttributes[c],l=[];for(let h=0,f=u.length;h<f;h++){let p=u[h];l.push(p.toJSON(e.data))}l.length>0&&(r[c]=l,s=!0)}s&&(e.data.morphAttributes=r,e.data.morphTargetsRelative=this.morphTargetsRelative);let a=this.groups;a.length>0&&(e.data.groups=JSON.parse(JSON.stringify(a)));let o=this.boundingSphere;return o!==null&&(e.data.boundingSphere=o.toJSON()),e}clone(){return new this.constructor().copy(this)}copy(e){this.index=null,this.attributes={},this.morphAttributes={},this.groups=[],this.boundingBox=null,this.boundingSphere=null;let t={};this.name=e.name;let i=e.index;i!==null&&this.setIndex(i.clone());let r=e.attributes;for(let u in r){let l=r[u];this.setAttribute(u,l.clone(t))}let s=e.morphAttributes;for(let u in s){let l=[],h=s[u];for(let f=0,p=h.length;f<p;f++)l.push(h[f].clone(t));this.morphAttributes[u]=l}this.morphTargetsRelative=e.morphTargetsRelative;let a=e.groups;for(let u=0,l=a.length;u<l;u++){let h=a[u];this.addGroup(h.start,h.count,h.materialIndex)}let o=e.boundingBox;o!==null&&(this.boundingBox=o.clone());let c=e.boundingSphere;return c!==null&&(this.boundingSphere=c.clone()),this.drawRange.start=e.drawRange.start,this.drawRange.count=e.drawRange.count,this.userData=e.userData,this}dispose(){this.dispatchEvent({type:"dispose"})}},bf=new _t,Xi=new er,Ja=new Ti,Mf=new L,Qa=new L,eo=new L,to=new L,Wl=new L,no=new L,Ef=new L,io=new L,Ht=class extends cn{constructor(e=new li,t=new Ai){super(),this.isMesh=!0,this.type="Mesh",this.geometry=e,this.material=t,this.morphTargetDictionary=void 0,this.morphTargetInfluences=void 0,this.count=1,this.updateMorphTargets()}copy(e,t){return super.copy(e,t),e.morphTargetInfluences!==void 0&&(this.morphTargetInfluences=e.morphTargetInfluences.slice()),e.morphTargetDictionary!==void 0&&(this.morphTargetDictionary=Object.assign({},e.morphTargetDictionary)),this.material=Array.isArray(e.material)?e.material.slice():e.material,this.geometry=e.geometry,this}updateMorphTargets(){let t=this.geometry.morphAttributes,i=Object.keys(t);if(i.length>0){let r=t[i[0]];if(r!==void 0){this.morphTargetInfluences=[],this.morphTargetDictionary={};for(let s=0,a=r.length;s<a;s++){let o=r[s].name||String(s);this.morphTargetInfluences.push(0),this.morphTargetDictionary[o]=s}}}}getVertexPosition(e,t){let i=this.geometry,r=i.attributes.position,s=i.morphAttributes.position,a=i.morphTargetsRelative;t.fromBufferAttribute(r,e);let o=this.morphTargetInfluences;if(s&&o){no.set(0,0,0);for(let c=0,u=s.length;c<u;c++){let l=o[c],h=s[c];l!==0&&(Wl.fromBufferAttribute(h,e),a?no.addScaledVector(Wl,l):no.addScaledVector(Wl.sub(t),l))}t.add(no)}return t}raycast(e,t){let i=this.geometry,r=this.material,s=this.matrixWorld;r!==void 0&&(i.boundingSphere===null&&i.computeBoundingSphere(),Ja.copy(i.boundingSphere),Ja.applyMatrix4(s),Xi.copy(e.ray).recast(e.near),!(Ja.containsPoint(Xi.origin)===!1&&(Xi.intersectSphere(Ja,Mf)===null||Xi.origin.distanceToSquared(Mf)>(e.far-e.near)**2))&&(bf.copy(s).invert(),Xi.copy(e.ray).applyMatrix4(bf),!(i.boundingBox!==null&&Xi.intersectsBox(i.boundingBox)===!1)&&this._computeIntersections(e,t,Xi)))}_computeIntersections(e,t,i){let r,s=this.geometry,a=this.material,o=s.index,c=s.attributes.position,u=s.attributes.uv,l=s.attributes.uv1,h=s.attributes.normal,f=s.groups,p=s.drawRange;if(o!==null)if(Array.isArray(a))for(let g=0,_=f.length;g<_;g++){let m=f[g],d=a[m.materialIndex],w=Math.max(m.start,p.start),M=Math.min(o.count,Math.min(m.start+m.count,p.start+p.count));for(let x=w,T=M;x<T;x+=3){let R=o.getX(x),P=o.getX(x+1),O=o.getX(x+2);r=ro(this,d,e,i,u,l,h,R,P,O),r&&(r.faceIndex=Math.floor(x/3),r.face.materialIndex=m.materialIndex,t.push(r))}}else{let g=Math.max(0,p.start),_=Math.min(o.count,p.start+p.count);for(let m=g,d=_;m<d;m+=3){let w=o.getX(m),M=o.getX(m+1),x=o.getX(m+2);r=ro(this,a,e,i,u,l,h,w,M,x),r&&(r.faceIndex=Math.floor(m/3),t.push(r))}}else if(c!==void 0)if(Array.isArray(a))for(let g=0,_=f.length;g<_;g++){let m=f[g],d=a[m.materialIndex],w=Math.max(m.start,p.start),M=Math.min(c.count,Math.min(m.start+m.count,p.start+p.count));for(let x=w,T=M;x<T;x+=3){let R=x,P=x+1,O=x+2;r=ro(this,d,e,i,u,l,h,R,P,O),r&&(r.faceIndex=Math.floor(x/3),r.face.materialIndex=m.materialIndex,t.push(r))}}else{let g=Math.max(0,p.start),_=Math.min(c.count,p.start+p.count);for(let m=g,d=_;m<d;m+=3){let w=m,M=m+1,x=m+2;r=ro(this,a,e,i,u,l,h,w,M,x),r&&(r.faceIndex=Math.floor(m/3),t.push(r))}}}};function Ey(n,e,t,i,r,s,a,o){let c;if(e.side===Jt?c=i.intersectTriangle(a,s,r,!0,o):c=i.intersectTriangle(r,s,a,e.side===ci,o),c===null)return null;io.copy(o),io.applyMatrix4(n.matrixWorld);let u=t.ray.origin.distanceTo(io);return u<t.near||u>t.far?null:{distance:u,point:io.clone(),object:n}}function ro(n,e,t,i,r,s,a,o,c,u){n.getVertexPosition(o,Qa),n.getVertexPosition(c,eo),n.getVertexPosition(u,to);let l=Ey(n,e,t,i,Qa,eo,to,Ef);if(l){let h=new L;Mi.getBarycoord(Ef,Qa,eo,to,h),r&&(l.uv=Mi.getInterpolatedAttribute(r,o,c,u,h,new Ue)),s&&(l.uv1=Mi.getInterpolatedAttribute(s,o,c,u,h,new Ue)),a&&(l.normal=Mi.getInterpolatedAttribute(a,o,c,u,h,new L),l.normal.dot(i.direction)>0&&l.normal.multiplyScalar(-1));let f={a:o,b:c,c:u,normal:new L,materialIndex:0};Mi.getNormal(Qa,eo,to,f.normal),l.face=f,l.barycoord=h}return l}var Yr=class n extends li{constructor(e=1,t=1,i=1,r=1,s=1,a=1){super(),this.type="BoxGeometry",this.parameters={width:e,height:t,depth:i,widthSegments:r,heightSegments:s,depthSegments:a};let o=this;r=Math.floor(r),s=Math.floor(s),a=Math.floor(a);let c=[],u=[],l=[],h=[],f=0,p=0;g("z","y","x",-1,-1,i,t,e,a,s,0),g("z","y","x",1,-1,i,t,-e,a,s,1),g("x","z","y",1,1,e,i,t,r,a,2),g("x","z","y",1,-1,e,i,-t,r,a,3),g("x","y","z",1,-1,e,t,i,r,s,4),g("x","y","z",-1,-1,e,t,-i,r,s,5),this.setIndex(c),this.setAttribute("position",new vn(u,3)),this.setAttribute("normal",new vn(l,3)),this.setAttribute("uv",new vn(h,2));function g(_,m,d,w,M,x,T,R,P,O,b){let S=x/P,I=T/O,k=x/2,V=T/2,W=R/2,j=P+1,G=O+1,ne=0,z=0,ae=new L;for(let ue=0;ue<G;ue++){let we=ue*I-V;for(let je=0;je<j;je++){let ft=je*S-k;ae[_]=ft*w,ae[m]=we*M,ae[d]=W,u.push(ae.x,ae.y,ae.z),ae[_]=0,ae[m]=0,ae[d]=R>0?1:-1,l.push(ae.x,ae.y,ae.z),h.push(je/P),h.push(1-ue/O),ne+=1}}for(let ue=0;ue<O;ue++)for(let we=0;we<P;we++){let je=f+we+j*ue,ft=f+we+j*(ue+1),xt=f+(we+1)+j*(ue+1),rt=f+(we+1)+j*ue;c.push(je,ft,rt),c.push(ft,xt,rt),z+=6}o.addGroup(p,z,b),p+=z,f+=ne}}copy(e){return super.copy(e),this.parameters=Object.assign({},e.parameters),this}static fromJSON(e){return new n(e.width,e.height,e.depth,e.widthSegments,e.heightSegments,e.depthSegments)}};function sr(n){let e={};for(let t in n){e[t]={};for(let i in n[t]){let r=n[t][i];r&&(r.isColor||r.isMatrix3||r.isMatrix4||r.isVector2||r.isVector3||r.isVector4||r.isTexture||r.isQuaternion)?r.isRenderTargetTexture?(console.warn("UniformsUtils: Textures of render targets cannot be cloned via cloneUniforms() or mergeUniforms()."),e[t][i]=null):e[t][i]=r.clone():Array.isArray(r)?e[t][i]=r.slice():e[t][i]=r}}return e}function Gt(n){let e={};for(let t=0;t<n.length;t++){let i=sr(n[t]);for(let r in i)e[r]=i[r]}return e}function wy(n){let e=[];for(let t=0;t<n.length;t++)e.push(n[t].clone());return e}function yu(n){let e=n.getRenderTarget();return e===null?n.outputColorSpace:e.isXRRenderTarget===!0?e.texture.colorSpace:tt.workingColorSpace}var bp={clone:sr,merge:Gt},Ty=`void main() {
	gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );
}`,Ay=`void main() {
	gl_FragColor = vec4( 1.0, 0.0, 0.0, 1.0 );
}`,Fn=class extends tr{constructor(e){super(),this.isShaderMaterial=!0,this.type="ShaderMaterial",this.defines={},this.uniforms={},this.uniformsGroups=[],this.vertexShader=Ty,this.fragmentShader=Ay,this.linewidth=1,this.wireframe=!1,this.wireframeLinewidth=1,this.fog=!1,this.lights=!1,this.clipping=!1,this.forceSinglePass=!0,this.extensions={clipCullDistance:!1,multiDraw:!1},this.defaultAttributeValues={color:[1,1,1],uv:[0,0],uv1:[0,0]},this.index0AttributeName=void 0,this.uniformsNeedUpdate=!1,this.glslVersion=null,e!==void 0&&this.setValues(e)}copy(e){return super.copy(e),this.fragmentShader=e.fragmentShader,this.vertexShader=e.vertexShader,this.uniforms=sr(e.uniforms),this.uniformsGroups=wy(e.uniformsGroups),this.defines=Object.assign({},e.defines),this.wireframe=e.wireframe,this.wireframeLinewidth=e.wireframeLinewidth,this.fog=e.fog,this.lights=e.lights,this.clipping=e.clipping,this.extensions=Object.assign({},e.extensions),this.glslVersion=e.glslVersion,this}toJSON(e){let t=super.toJSON(e);t.glslVersion=this.glslVersion,t.uniforms={};for(let r in this.uniforms){let a=this.uniforms[r].value;a&&a.isTexture?t.uniforms[r]={type:"t",value:a.toJSON(e).uuid}:a&&a.isColor?t.uniforms[r]={type:"c",value:a.getHex()}:a&&a.isVector2?t.uniforms[r]={type:"v2",value:a.toArray()}:a&&a.isVector3?t.uniforms[r]={type:"v3",value:a.toArray()}:a&&a.isVector4?t.uniforms[r]={type:"v4",value:a.toArray()}:a&&a.isMatrix3?t.uniforms[r]={type:"m3",value:a.toArray()}:a&&a.isMatrix4?t.uniforms[r]={type:"m4",value:a.toArray()}:t.uniforms[r]={value:a}}Object.keys(this.defines).length>0&&(t.defines=this.defines),t.vertexShader=this.vertexShader,t.fragmentShader=this.fragmentShader,t.lights=this.lights,t.clipping=this.clipping;let i={};for(let r in this.extensions)this.extensions[r]===!0&&(i[r]=!0);return Object.keys(i).length>0&&(t.extensions=i),t}},zs=class extends cn{constructor(){super(),this.isCamera=!0,this.type="Camera",this.matrixWorldInverse=new _t,this.projectionMatrix=new _t,this.projectionMatrixInverse=new _t,this.coordinateSystem=Un,this._reversedDepth=!1}get reversedDepth(){return this._reversedDepth}copy(e,t){return super.copy(e,t),this.matrixWorldInverse.copy(e.matrixWorldInverse),this.projectionMatrix.copy(e.projectionMatrix),this.projectionMatrixInverse.copy(e.projectionMatrixInverse),this.coordinateSystem=e.coordinateSystem,this}getWorldDirection(e){return super.getWorldDirection(e).negate()}updateMatrixWorld(e){super.updateMatrixWorld(e),this.matrixWorldInverse.copy(this.matrixWorld).invert()}updateWorldMatrix(e,t){super.updateWorldMatrix(e,t),this.matrixWorldInverse.copy(this.matrixWorld).invert()}clone(){return new this.constructor().copy(this)}},bi=new L,wf=new Ue,Tf=new Ue,$t=class extends zs{constructor(e=50,t=1,i=.1,r=2e3){super(),this.isPerspectiveCamera=!0,this.type="PerspectiveCamera",this.fov=e,this.zoom=1,this.near=i,this.far=r,this.focus=10,this.aspect=t,this.view=null,this.filmGauge=35,this.filmOffset=0,this.updateProjectionMatrix()}copy(e,t){return super.copy(e,t),this.fov=e.fov,this.zoom=e.zoom,this.near=e.near,this.far=e.far,this.focus=e.focus,this.aspect=e.aspect,this.view=e.view===null?null:Object.assign({},e.view),this.filmGauge=e.filmGauge,this.filmOffset=e.filmOffset,this}setFocalLength(e){let t=.5*this.getFilmHeight()/e;this.fov=Wr*2*Math.atan(t),this.updateProjectionMatrix()}getFocalLength(){let e=Math.tan(Is*.5*this.fov);return .5*this.getFilmHeight()/e}getEffectiveFOV(){return Wr*2*Math.atan(Math.tan(Is*.5*this.fov)/this.zoom)}getFilmWidth(){return this.filmGauge*Math.min(this.aspect,1)}getFilmHeight(){return this.filmGauge/Math.max(this.aspect,1)}getViewBounds(e,t,i){bi.set(-1,-1,.5).applyMatrix4(this.projectionMatrixInverse),t.set(bi.x,bi.y).multiplyScalar(-e/bi.z),bi.set(1,1,.5).applyMatrix4(this.projectionMatrixInverse),i.set(bi.x,bi.y).multiplyScalar(-e/bi.z)}getViewSize(e,t){return this.getViewBounds(e,wf,Tf),t.subVectors(Tf,wf)}setViewOffset(e,t,i,r,s,a){this.aspect=e/t,this.view===null&&(this.view={enabled:!0,fullWidth:1,fullHeight:1,offsetX:0,offsetY:0,width:1,height:1}),this.view.enabled=!0,this.view.fullWidth=e,this.view.fullHeight=t,this.view.offsetX=i,this.view.offsetY=r,this.view.width=s,this.view.height=a,this.updateProjectionMatrix()}clearViewOffset(){this.view!==null&&(this.view.enabled=!1),this.updateProjectionMatrix()}updateProjectionMatrix(){let e=this.near,t=e*Math.tan(Is*.5*this.fov)/this.zoom,i=2*t,r=this.aspect*i,s=-.5*r,a=this.view;if(this.view!==null&&this.view.enabled){let c=a.fullWidth,u=a.fullHeight;s+=a.offsetX*r/c,t-=a.offsetY*i/u,r*=a.width/c,i*=a.height/u}let o=this.filmOffset;o!==0&&(s+=e*o/this.getFilmWidth()),this.projectionMatrix.makePerspective(s,s+r,t,t-i,e,this.far,this.coordinateSystem,this.reversedDepth),this.projectionMatrixInverse.copy(this.projectionMatrix).invert()}toJSON(e){let t=super.toJSON(e);return t.object.fov=this.fov,t.object.zoom=this.zoom,t.object.near=this.near,t.object.far=this.far,t.object.focus=this.focus,t.object.aspect=this.aspect,this.view!==null&&(t.object.view=Object.assign({},this.view)),t.object.filmGauge=this.filmGauge,t.object.filmOffset=this.filmOffset,t}},kr=-90,zr=1,yo=class extends cn{constructor(e,t,i){super(),this.type="CubeCamera",this.renderTarget=i,this.coordinateSystem=null,this.activeMipmapLevel=0;let r=new $t(kr,zr,e,t);r.layers=this.layers,this.add(r);let s=new $t(kr,zr,e,t);s.layers=this.layers,this.add(s);let a=new $t(kr,zr,e,t);a.layers=this.layers,this.add(a);let o=new $t(kr,zr,e,t);o.layers=this.layers,this.add(o);let c=new $t(kr,zr,e,t);c.layers=this.layers,this.add(c);let u=new $t(kr,zr,e,t);u.layers=this.layers,this.add(u)}updateCoordinateSystem(){let e=this.coordinateSystem,t=this.children.concat(),[i,r,s,a,o,c]=t;for(let u of t)this.remove(u);if(e===Un)i.up.set(0,1,0),i.lookAt(1,0,0),r.up.set(0,1,0),r.lookAt(-1,0,0),s.up.set(0,0,-1),s.lookAt(0,1,0),a.up.set(0,0,1),a.lookAt(0,-1,0),o.up.set(0,1,0),o.lookAt(0,0,1),c.up.set(0,1,0),c.lookAt(0,0,-1);else if(e===Us)i.up.set(0,-1,0),i.lookAt(-1,0,0),r.up.set(0,-1,0),r.lookAt(1,0,0),s.up.set(0,0,1),s.lookAt(0,1,0),a.up.set(0,0,-1),a.lookAt(0,-1,0),o.up.set(0,-1,0),o.lookAt(0,0,1),c.up.set(0,-1,0),c.lookAt(0,0,-1);else throw new Error("THREE.CubeCamera.updateCoordinateSystem(): Invalid coordinate system: "+e);for(let u of t)this.add(u),u.updateMatrixWorld()}update(e,t){this.parent===null&&this.updateMatrixWorld();let{renderTarget:i,activeMipmapLevel:r}=this;this.coordinateSystem!==e.coordinateSystem&&(this.coordinateSystem=e.coordinateSystem,this.updateCoordinateSystem());let[s,a,o,c,u,l]=this.children,h=e.getRenderTarget(),f=e.getActiveCubeFace(),p=e.getActiveMipmapLevel(),g=e.xr.enabled;e.xr.enabled=!1;let _=i.texture.generateMipmaps;i.texture.generateMipmaps=!1,e.setRenderTarget(i,0,r),e.render(t,s),e.setRenderTarget(i,1,r),e.render(t,a),e.setRenderTarget(i,2,r),e.render(t,o),e.setRenderTarget(i,3,r),e.render(t,c),e.setRenderTarget(i,4,r),e.render(t,u),i.texture.generateMipmaps=_,e.setRenderTarget(i,5,r),e.render(t,l),e.setRenderTarget(h,f,p),e.xr.enabled=g,i.texture.needsPMREMUpdate=!0}},Vs=class extends on{constructor(e=[],t=ir,i,r,s,a,o,c,u,l){super(e,t,i,r,s,a,o,c,u,l),this.isCubeTexture=!0,this.flipY=!1}get images(){return this.image}set images(e){this.image=e}},vo=class extends Wn{constructor(e=1,t={}){super(e,e,t),this.isWebGLCubeRenderTarget=!0;let i={width:e,height:e,depth:1},r=[i,i,i,i,i,i];this.texture=new Vs(r),this._setTextureOptions(t),this.texture.isRenderTargetTexture=!0}fromEquirectangularTexture(e,t){this.texture.type=t.type,this.texture.colorSpace=t.colorSpace,this.texture.generateMipmaps=t.generateMipmaps,this.texture.minFilter=t.minFilter,this.texture.magFilter=t.magFilter;let i={uniforms:{tEquirect:{value:null}},vertexShader:`

				varying vec3 vWorldDirection;

				vec3 transformDirection( in vec3 dir, in mat4 matrix ) {

					return normalize( ( matrix * vec4( dir, 0.0 ) ).xyz );

				}

				void main() {

					vWorldDirection = transformDirection( position, modelMatrix );

					#include <begin_vertex>
					#include <project_vertex>

				}
			`,fragmentShader:`

				uniform sampler2D tEquirect;

				varying vec3 vWorldDirection;

				#include <common>

				void main() {

					vec3 direction = normalize( vWorldDirection );

					vec2 sampleUV = equirectUv( direction );

					gl_FragColor = texture2D( tEquirect, sampleUV );

				}
			`},r=new Yr(5,5,5),s=new Fn({name:"CubemapFromEquirect",uniforms:sr(i.uniforms),vertexShader:i.vertexShader,fragmentShader:i.fragmentShader,side:Jt,blending:ui});s.uniforms.tEquirect.value=t;let a=new Ht(r,s),o=t.minFilter;return t.minFilter===Di&&(t.minFilter=On),new yo(1,10,this).update(e,a),t.minFilter=o,a.geometry.dispose(),a.material.dispose(),this}clear(e,t=!0,i=!0,r=!0){let s=e.getRenderTarget();for(let a=0;a<6;a++)e.setRenderTarget(this,a),e.clear(t,i,r);e.setRenderTarget(s)}},Zi=class extends cn{constructor(){super(),this.isGroup=!0,this.type="Group"}},Ry={type:"move"},Zr=class{constructor(){this._targetRay=null,this._grip=null,this._hand=null}getHandSpace(){return this._hand===null&&(this._hand=new Zi,this._hand.matrixAutoUpdate=!1,this._hand.visible=!1,this._hand.joints={},this._hand.inputState={pinching:!1}),this._hand}getTargetRaySpace(){return this._targetRay===null&&(this._targetRay=new Zi,this._targetRay.matrixAutoUpdate=!1,this._targetRay.visible=!1,this._targetRay.hasLinearVelocity=!1,this._targetRay.linearVelocity=new L,this._targetRay.hasAngularVelocity=!1,this._targetRay.angularVelocity=new L),this._targetRay}getGripSpace(){return this._grip===null&&(this._grip=new Zi,this._grip.matrixAutoUpdate=!1,this._grip.visible=!1,this._grip.hasLinearVelocity=!1,this._grip.linearVelocity=new L,this._grip.hasAngularVelocity=!1,this._grip.angularVelocity=new L),this._grip}dispatchEvent(e){return this._targetRay!==null&&this._targetRay.dispatchEvent(e),this._grip!==null&&this._grip.dispatchEvent(e),this._hand!==null&&this._hand.dispatchEvent(e),this}connect(e){if(e&&e.hand){let t=this._hand;if(t)for(let i of e.hand.values())this._getHandJoint(t,i)}return this.dispatchEvent({type:"connected",data:e}),this}disconnect(e){return this.dispatchEvent({type:"disconnected",data:e}),this._targetRay!==null&&(this._targetRay.visible=!1),this._grip!==null&&(this._grip.visible=!1),this._hand!==null&&(this._hand.visible=!1),this}update(e,t,i){let r=null,s=null,a=null,o=this._targetRay,c=this._grip,u=this._hand;if(e&&t.session.visibilityState!=="visible-blurred"){if(u&&e.hand){a=!0;for(let _ of e.hand.values()){let m=t.getJointPose(_,i),d=this._getHandJoint(u,_);m!==null&&(d.matrix.fromArray(m.transform.matrix),d.matrix.decompose(d.position,d.rotation,d.scale),d.matrixWorldNeedsUpdate=!0,d.jointRadius=m.radius),d.visible=m!==null}let l=u.joints["index-finger-tip"],h=u.joints["thumb-tip"],f=l.position.distanceTo(h.position),p=.02,g=.005;u.inputState.pinching&&f>p+g?(u.inputState.pinching=!1,this.dispatchEvent({type:"pinchend",handedness:e.handedness,target:this})):!u.inputState.pinching&&f<=p-g&&(u.inputState.pinching=!0,this.dispatchEvent({type:"pinchstart",handedness:e.handedness,target:this}))}else c!==null&&e.gripSpace&&(s=t.getPose(e.gripSpace,i),s!==null&&(c.matrix.fromArray(s.transform.matrix),c.matrix.decompose(c.position,c.rotation,c.scale),c.matrixWorldNeedsUpdate=!0,s.linearVelocity?(c.hasLinearVelocity=!0,c.linearVelocity.copy(s.linearVelocity)):c.hasLinearVelocity=!1,s.angularVelocity?(c.hasAngularVelocity=!0,c.angularVelocity.copy(s.angularVelocity)):c.hasAngularVelocity=!1));o!==null&&(r=t.getPose(e.targetRaySpace,i),r===null&&s!==null&&(r=s),r!==null&&(o.matrix.fromArray(r.transform.matrix),o.matrix.decompose(o.position,o.rotation,o.scale),o.matrixWorldNeedsUpdate=!0,r.linearVelocity?(o.hasLinearVelocity=!0,o.linearVelocity.copy(r.linearVelocity)):o.hasLinearVelocity=!1,r.angularVelocity?(o.hasAngularVelocity=!0,o.angularVelocity.copy(r.angularVelocity)):o.hasAngularVelocity=!1,this.dispatchEvent(Ry)))}return o!==null&&(o.visible=r!==null),c!==null&&(c.visible=s!==null),u!==null&&(u.visible=a!==null),this}_getHandJoint(e,t){if(e.joints[t.jointName]===void 0){let i=new Zi;i.matrixAutoUpdate=!1,i.visible=!1,e.joints[t.jointName]=i,e.add(i)}return e.joints[t.jointName]}};var $s=class extends cn{constructor(){super(),this.isScene=!0,this.type="Scene",this.background=null,this.environment=null,this.fog=null,this.backgroundBlurriness=0,this.backgroundIntensity=1,this.backgroundRotation=new Xn,this.environmentIntensity=1,this.environmentRotation=new Xn,this.overrideMaterial=null,typeof __THREE_DEVTOOLS__<"u"&&__THREE_DEVTOOLS__.dispatchEvent(new CustomEvent("observe",{detail:this}))}copy(e,t){return super.copy(e,t),e.background!==null&&(this.background=e.background.clone()),e.environment!==null&&(this.environment=e.environment.clone()),e.fog!==null&&(this.fog=e.fog.clone()),this.backgroundBlurriness=e.backgroundBlurriness,this.backgroundIntensity=e.backgroundIntensity,this.backgroundRotation.copy(e.backgroundRotation),this.environmentIntensity=e.environmentIntensity,this.environmentRotation.copy(e.environmentRotation),e.overrideMaterial!==null&&(this.overrideMaterial=e.overrideMaterial.clone()),this.matrixAutoUpdate=e.matrixAutoUpdate,this}toJSON(e){let t=super.toJSON(e);return this.fog!==null&&(t.object.fog=this.fog.toJSON()),this.backgroundBlurriness>0&&(t.object.backgroundBlurriness=this.backgroundBlurriness),this.backgroundIntensity!==1&&(t.object.backgroundIntensity=this.backgroundIntensity),t.object.backgroundRotation=this.backgroundRotation.toArray(),this.environmentIntensity!==1&&(t.object.environmentIntensity=this.environmentIntensity),t.object.environmentRotation=this.environmentRotation.toArray(),t}};var xo=class extends on{constructor(e=null,t=1,i=1,r,s,a,o,c,u=sn,l=sn,h,f){super(null,a,o,c,u,l,r,s,h,f),this.isDataTexture=!0,this.image={data:e,width:t,height:i},this.generateMipmaps=!1,this.flipY=!1,this.unpackAlignment=1}};var Hs=class extends rn{constructor(e,t,i,r=1){super(e,t,i),this.isInstancedBufferAttribute=!0,this.meshPerAttribute=r}copy(e){return super.copy(e),this.meshPerAttribute=e.meshPerAttribute,this}toJSON(){let e=super.toJSON();return e.meshPerAttribute=this.meshPerAttribute,e.isInstancedBufferAttribute=!0,e}},Vr=new _t,Af=new _t,so=[],Rf=new qn,Cy=new _t,Cs=new Ht,Ps=new Ti,Gs=class extends Ht{constructor(e,t,i){super(e,t),this.isInstancedMesh=!0,this.instanceMatrix=new Hs(new Float32Array(i*16),16),this.instanceColor=null,this.morphTexture=null,this.count=i,this.boundingBox=null,this.boundingSphere=null;for(let r=0;r<i;r++)this.setMatrixAt(r,Cy)}computeBoundingBox(){let e=this.geometry,t=this.count;this.boundingBox===null&&(this.boundingBox=new qn),e.boundingBox===null&&e.computeBoundingBox(),this.boundingBox.makeEmpty();for(let i=0;i<t;i++)this.getMatrixAt(i,Vr),Rf.copy(e.boundingBox).applyMatrix4(Vr),this.boundingBox.union(Rf)}computeBoundingSphere(){let e=this.geometry,t=this.count;this.boundingSphere===null&&(this.boundingSphere=new Ti),e.boundingSphere===null&&e.computeBoundingSphere(),this.boundingSphere.makeEmpty();for(let i=0;i<t;i++)this.getMatrixAt(i,Vr),Ps.copy(e.boundingSphere).applyMatrix4(Vr),this.boundingSphere.union(Ps)}copy(e,t){return super.copy(e,t),this.instanceMatrix.copy(e.instanceMatrix),e.morphTexture!==null&&(this.morphTexture=e.morphTexture.clone()),e.instanceColor!==null&&(this.instanceColor=e.instanceColor.clone()),this.count=e.count,e.boundingBox!==null&&(this.boundingBox=e.boundingBox.clone()),e.boundingSphere!==null&&(this.boundingSphere=e.boundingSphere.clone()),this}getColorAt(e,t){t.fromArray(this.instanceColor.array,e*3)}getMatrixAt(e,t){t.fromArray(this.instanceMatrix.array,e*16)}getMorphAt(e,t){let i=t.morphTargetInfluences,r=this.morphTexture.source.data.data,s=i.length+1,a=e*s+1;for(let o=0;o<i.length;o++)i[o]=r[a+o]}raycast(e,t){let i=this.matrixWorld,r=this.count;if(Cs.geometry=this.geometry,Cs.material=this.material,Cs.material!==void 0&&(this.boundingSphere===null&&this.computeBoundingSphere(),Ps.copy(this.boundingSphere),Ps.applyMatrix4(i),e.ray.intersectsSphere(Ps)!==!1))for(let s=0;s<r;s++){this.getMatrixAt(s,Vr),Af.multiplyMatrices(i,Vr),Cs.matrixWorld=Af,Cs.raycast(e,so);for(let a=0,o=so.length;a<o;a++){let c=so[a];c.instanceId=s,c.object=this,t.push(c)}so.length=0}}setColorAt(e,t){this.instanceColor===null&&(this.instanceColor=new Hs(new Float32Array(this.instanceMatrix.count*3).fill(1),3)),t.toArray(this.instanceColor.array,e*3)}setMatrixAt(e,t){t.toArray(this.instanceMatrix.array,e*16)}setMorphAt(e,t){let i=t.morphTargetInfluences,r=i.length+1;this.morphTexture===null&&(this.morphTexture=new xo(new Float32Array(r*this.count),r,this.count,Xo,Bn));let s=this.morphTexture.source.data.data,a=0;for(let u=0;u<i.length;u++)a+=i[u];let o=this.geometry.morphTargetsRelative?1:1-a,c=r*e;s[c]=o,s.set(i,c+1)}updateMorphTargets(){}dispose(){this.dispatchEvent({type:"dispose"}),this.morphTexture!==null&&(this.morphTexture.dispose(),this.morphTexture=null)}},ql=new L,Py=new L,Iy=new Fe,wn=class{constructor(e=new L(1,0,0),t=0){this.isPlane=!0,this.normal=e,this.constant=t}set(e,t){return this.normal.copy(e),this.constant=t,this}setComponents(e,t,i,r){return this.normal.set(e,t,i),this.constant=r,this}setFromNormalAndCoplanarPoint(e,t){return this.normal.copy(e),this.constant=-t.dot(this.normal),this}setFromCoplanarPoints(e,t,i){let r=ql.subVectors(i,t).cross(Py.subVectors(e,t)).normalize();return this.setFromNormalAndCoplanarPoint(r,e),this}copy(e){return this.normal.copy(e.normal),this.constant=e.constant,this}normalize(){let e=1/this.normal.length();return this.normal.multiplyScalar(e),this.constant*=e,this}negate(){return this.constant*=-1,this.normal.negate(),this}distanceToPoint(e){return this.normal.dot(e)+this.constant}distanceToSphere(e){return this.distanceToPoint(e.center)-e.radius}projectPoint(e,t){return t.copy(e).addScaledVector(this.normal,-this.distanceToPoint(e))}intersectLine(e,t){let i=e.delta(ql),r=this.normal.dot(i);if(r===0)return this.distanceToPoint(e.start)===0?t.copy(e.start):null;let s=-(e.start.dot(this.normal)+this.constant)/r;return s<0||s>1?null:t.copy(e.start).addScaledVector(i,s)}intersectsLine(e){let t=this.distanceToPoint(e.start),i=this.distanceToPoint(e.end);return t<0&&i>0||i<0&&t>0}intersectsBox(e){return e.intersectsPlane(this)}intersectsSphere(e){return e.intersectsPlane(this)}coplanarPoint(e){return e.copy(this.normal).multiplyScalar(-this.constant)}applyMatrix4(e,t){let i=t||Iy.getNormalMatrix(e),r=this.coplanarPoint(ql).applyMatrix4(e),s=this.normal.applyMatrix3(i).normalize();return this.constant=-r.dot(s),this}translate(e){return this.constant-=e.dot(this.normal),this}equals(e){return e.normal.equals(this.normal)&&e.constant===this.constant}clone(){return new this.constructor().copy(this)}},ji=new Ti,Dy=new Ue(.5,.5),ao=new L,Ws=class{constructor(e=new wn,t=new wn,i=new wn,r=new wn,s=new wn,a=new wn){this.planes=[e,t,i,r,s,a]}set(e,t,i,r,s,a){let o=this.planes;return o[0].copy(e),o[1].copy(t),o[2].copy(i),o[3].copy(r),o[4].copy(s),o[5].copy(a),this}copy(e){let t=this.planes;for(let i=0;i<6;i++)t[i].copy(e.planes[i]);return this}setFromProjectionMatrix(e,t=Un,i=!1){let r=this.planes,s=e.elements,a=s[0],o=s[1],c=s[2],u=s[3],l=s[4],h=s[5],f=s[6],p=s[7],g=s[8],_=s[9],m=s[10],d=s[11],w=s[12],M=s[13],x=s[14],T=s[15];if(r[0].setComponents(u-a,p-l,d-g,T-w).normalize(),r[1].setComponents(u+a,p+l,d+g,T+w).normalize(),r[2].setComponents(u+o,p+h,d+_,T+M).normalize(),r[3].setComponents(u-o,p-h,d-_,T-M).normalize(),i)r[4].setComponents(c,f,m,x).normalize(),r[5].setComponents(u-c,p-f,d-m,T-x).normalize();else if(r[4].setComponents(u-c,p-f,d-m,T-x).normalize(),t===Un)r[5].setComponents(u+c,p+f,d+m,T+x).normalize();else if(t===Us)r[5].setComponents(c,f,m,x).normalize();else throw new Error("THREE.Frustum.setFromProjectionMatrix(): Invalid coordinate system: "+t);return this}intersectsObject(e){if(e.boundingSphere!==void 0)e.boundingSphere===null&&e.computeBoundingSphere(),ji.copy(e.boundingSphere).applyMatrix4(e.matrixWorld);else{let t=e.geometry;t.boundingSphere===null&&t.computeBoundingSphere(),ji.copy(t.boundingSphere).applyMatrix4(e.matrixWorld)}return this.intersectsSphere(ji)}intersectsSprite(e){ji.center.set(0,0,0);let t=Dy.distanceTo(e.center);return ji.radius=.7071067811865476+t,ji.applyMatrix4(e.matrixWorld),this.intersectsSphere(ji)}intersectsSphere(e){let t=this.planes,i=e.center,r=-e.radius;for(let s=0;s<6;s++)if(t[s].distanceToPoint(i)<r)return!1;return!0}intersectsBox(e){let t=this.planes;for(let i=0;i<6;i++){let r=t[i];if(ao.x=r.normal.x>0?e.max.x:e.min.x,ao.y=r.normal.y>0?e.max.y:e.min.y,ao.z=r.normal.z>0?e.max.z:e.min.z,r.distanceToPoint(ao)<0)return!1}return!0}containsPoint(e){let t=this.planes;for(let i=0;i<6;i++)if(t[i].distanceToPoint(e)<0)return!1;return!0}clone(){return new this.constructor().copy(this)}};var qs=class extends on{constructor(e,t,i=Ni,r,s,a,o=sn,c=sn,u,l=Gr,h=1){if(l!==Gr&&l!==ts)throw new Error("DepthTexture format must be either THREE.DepthFormat or THREE.DepthStencilFormat");let f={width:e,height:t,depth:h};super(f,r,s,a,o,c,l,i,u),this.isDepthTexture=!0,this.flipY=!1,this.generateMipmaps=!1,this.compareFunction=null}copy(e){return super.copy(e),this.source=new Xr(Object.assign({},e.image)),this.compareFunction=e.compareFunction,this}toJSON(e){let t=super.toJSON(e);return this.compareFunction!==null&&(t.compareFunction=this.compareFunction),t}},Xs=class extends on{constructor(e=null){super(),this.sourceTexture=e,this.isExternalTexture=!0}copy(e){return super.copy(e),this.sourceTexture=e.sourceTexture,this}};var js=class n extends li{constructor(e=1,t=1,i=1,r=1){super(),this.type="PlaneGeometry",this.parameters={width:e,height:t,widthSegments:i,heightSegments:r};let s=e/2,a=t/2,o=Math.floor(i),c=Math.floor(r),u=o+1,l=c+1,h=e/o,f=t/c,p=[],g=[],_=[],m=[];for(let d=0;d<l;d++){let w=d*f-a;for(let M=0;M<u;M++){let x=M*h-s;g.push(x,-w,0),_.push(0,0,1),m.push(M/o),m.push(1-d/c)}}for(let d=0;d<c;d++)for(let w=0;w<o;w++){let M=w+u*d,x=w+u*(d+1),T=w+1+u*(d+1),R=w+1+u*d;p.push(M,x,R),p.push(x,T,R)}this.setIndex(p),this.setAttribute("position",new vn(g,3)),this.setAttribute("normal",new vn(_,3)),this.setAttribute("uv",new vn(m,2))}copy(e){return super.copy(e),this.parameters=Object.assign({},e.parameters),this}static fromJSON(e){return new n(e.width,e.height,e.widthSegments,e.heightSegments)}};var Ys=class n extends li{constructor(e=1,t=32,i=16,r=0,s=Math.PI*2,a=0,o=Math.PI){super(),this.type="SphereGeometry",this.parameters={radius:e,widthSegments:t,heightSegments:i,phiStart:r,phiLength:s,thetaStart:a,thetaLength:o},t=Math.max(3,Math.floor(t)),i=Math.max(2,Math.floor(i));let c=Math.min(a+o,Math.PI),u=0,l=[],h=new L,f=new L,p=[],g=[],_=[],m=[];for(let d=0;d<=i;d++){let w=[],M=d/i,x=0;d===0&&a===0?x=.5/t:d===i&&c===Math.PI&&(x=-.5/t);for(let T=0;T<=t;T++){let R=T/t;h.x=-e*Math.cos(r+R*s)*Math.sin(a+M*o),h.y=e*Math.cos(a+M*o),h.z=e*Math.sin(r+R*s)*Math.sin(a+M*o),g.push(h.x,h.y,h.z),f.copy(h).normalize(),_.push(f.x,f.y,f.z),m.push(R+x,1-M),w.push(u++)}l.push(w)}for(let d=0;d<i;d++)for(let w=0;w<t;w++){let M=l[d][w+1],x=l[d][w],T=l[d+1][w],R=l[d+1][w+1];(d!==0||a>0)&&p.push(M,x,R),(d!==i-1||c<Math.PI)&&p.push(x,T,R)}this.setIndex(p),this.setAttribute("position",new vn(g,3)),this.setAttribute("normal",new vn(_,3)),this.setAttribute("uv",new vn(m,2))}copy(e){return super.copy(e),this.parameters=Object.assign({},e.parameters),this}static fromJSON(e){return new n(e.radius,e.widthSegments,e.heightSegments,e.phiStart,e.phiLength,e.thetaStart,e.thetaLength)}};var So=class extends tr{constructor(e){super(),this.isMeshDepthMaterial=!0,this.type="MeshDepthMaterial",this.depthPacking=cp,this.map=null,this.alphaMap=null,this.displacementMap=null,this.displacementScale=1,this.displacementBias=0,this.wireframe=!1,this.wireframeLinewidth=1,this.setValues(e)}copy(e){return super.copy(e),this.depthPacking=e.depthPacking,this.map=e.map,this.alphaMap=e.alphaMap,this.displacementMap=e.displacementMap,this.displacementScale=e.displacementScale,this.displacementBias=e.displacementBias,this.wireframe=e.wireframe,this.wireframeLinewidth=e.wireframeLinewidth,this}},bo=class extends tr{constructor(e){super(),this.isMeshDistanceMaterial=!0,this.type="MeshDistanceMaterial",this.map=null,this.alphaMap=null,this.displacementMap=null,this.displacementScale=1,this.displacementBias=0,this.setValues(e)}copy(e){return super.copy(e),this.map=e.map,this.alphaMap=e.alphaMap,this.displacementMap=e.displacementMap,this.displacementScale=e.displacementScale,this.displacementBias=e.displacementBias,this}};function oo(n,e){return!n||n.constructor===e?n:typeof e.BYTES_PER_ELEMENT=="number"?new e(n):Array.prototype.slice.call(n)}function Ny(n){return ArrayBuffer.isView(n)&&!(n instanceof DataView)}var nr=class{constructor(e,t,i,r){this.parameterPositions=e,this._cachedIndex=0,this.resultBuffer=r!==void 0?r:new t.constructor(i),this.sampleValues=t,this.valueSize=i,this.settings=null,this.DefaultSettings_={}}evaluate(e){let t=this.parameterPositions,i=this._cachedIndex,r=t[i],s=t[i-1];n:{e:{let a;t:{i:if(!(e<r)){for(let o=i+2;;){if(r===void 0){if(e<s)break i;return i=t.length,this._cachedIndex=i,this.copySampleValue_(i-1)}if(i===o)break;if(s=r,r=t[++i],e<r)break e}a=t.length;break t}if(!(e>=s)){let o=t[1];e<o&&(i=2,s=o);for(let c=i-2;;){if(s===void 0)return this._cachedIndex=0,this.copySampleValue_(0);if(i===c)break;if(r=s,s=t[--i-1],e>=s)break e}a=i,i=0;break t}break n}for(;i<a;){let o=i+a>>>1;e<t[o]?a=o:i=o+1}if(r=t[i],s=t[i-1],s===void 0)return this._cachedIndex=0,this.copySampleValue_(0);if(r===void 0)return i=t.length,this._cachedIndex=i,this.copySampleValue_(i-1)}this._cachedIndex=i,this.intervalChanged_(i,s,r)}return this.interpolate_(i,s,e,r)}getSettings_(){return this.settings||this.DefaultSettings_}copySampleValue_(e){let t=this.resultBuffer,i=this.sampleValues,r=this.valueSize,s=e*r;for(let a=0;a!==r;++a)t[a]=i[s+a];return t}interpolate_(){throw new Error("call to abstract method")}intervalChanged_(){}},Mo=class extends nr{constructor(e,t,i,r){super(e,t,i,r),this._weightPrev=-0,this._offsetPrev=-0,this._weightNext=-0,this._offsetNext=-0,this.DefaultSettings_={endingStart:Xl,endingEnd:Xl}}intervalChanged_(e,t,i){let r=this.parameterPositions,s=e-2,a=e+1,o=r[s],c=r[a];if(o===void 0)switch(this.getSettings_().endingStart){case jl:s=e,o=2*t-i;break;case Yl:s=r.length-2,o=t+r[s]-r[s+1];break;default:s=e,o=i}if(c===void 0)switch(this.getSettings_().endingEnd){case jl:a=e,c=2*i-t;break;case Yl:a=1,c=i+r[1]-r[0];break;default:a=e-1,c=t}let u=(i-t)*.5,l=this.valueSize;this._weightPrev=u/(t-o),this._weightNext=u/(c-i),this._offsetPrev=s*l,this._offsetNext=a*l}interpolate_(e,t,i,r){let s=this.resultBuffer,a=this.sampleValues,o=this.valueSize,c=e*o,u=c-o,l=this._offsetPrev,h=this._offsetNext,f=this._weightPrev,p=this._weightNext,g=(i-t)/(r-t),_=g*g,m=_*g,d=-f*m+2*f*_-f*g,w=(1+f)*m+(-1.5-2*f)*_+(-.5+f)*g+1,M=(-1-p)*m+(1.5+p)*_+.5*g,x=p*m-p*_;for(let T=0;T!==o;++T)s[T]=d*a[l+T]+w*a[u+T]+M*a[c+T]+x*a[h+T];return s}},Eo=class extends nr{constructor(e,t,i,r){super(e,t,i,r)}interpolate_(e,t,i,r){let s=this.resultBuffer,a=this.sampleValues,o=this.valueSize,c=e*o,u=c-o,l=(i-t)/(r-t),h=1-l;for(let f=0;f!==o;++f)s[f]=a[u+f]*h+a[c+f]*l;return s}},wo=class extends nr{constructor(e,t,i,r){super(e,t,i,r)}interpolate_(e){return this.copySampleValue_(e-1)}},xn=class{constructor(e,t,i,r){if(e===void 0)throw new Error("THREE.KeyframeTrack: track name is undefined");if(t===void 0||t.length===0)throw new Error("THREE.KeyframeTrack: no keyframes in track named "+e);this.name=e,this.times=oo(t,this.TimeBufferType),this.values=oo(i,this.ValueBufferType),this.setInterpolation(r||this.DefaultInterpolation)}static toJSON(e){let t=e.constructor,i;if(t.toJSON!==this.toJSON)i=t.toJSON(e);else{i={name:e.name,times:oo(e.times,Array),values:oo(e.values,Array)};let r=e.getInterpolation();r!==e.DefaultInterpolation&&(i.interpolation=r)}return i.type=e.ValueTypeName,i}InterpolantFactoryMethodDiscrete(e){return new wo(this.times,this.values,this.getValueSize(),e)}InterpolantFactoryMethodLinear(e){return new Eo(this.times,this.values,this.getValueSize(),e)}InterpolantFactoryMethodSmooth(e){return new Mo(this.times,this.values,this.getValueSize(),e)}setInterpolation(e){let t;switch(e){case Ns:t=this.InterpolantFactoryMethodDiscrete;break;case po:t=this.InterpolantFactoryMethodLinear;break;case co:t=this.InterpolantFactoryMethodSmooth;break}if(t===void 0){let i="unsupported interpolation for "+this.ValueTypeName+" keyframe track named "+this.name;if(this.createInterpolant===void 0)if(e!==this.DefaultInterpolation)this.setInterpolation(this.DefaultInterpolation);else throw new Error(i);return console.warn("THREE.KeyframeTrack:",i),this}return this.createInterpolant=t,this}getInterpolation(){switch(this.createInterpolant){case this.InterpolantFactoryMethodDiscrete:return Ns;case this.InterpolantFactoryMethodLinear:return po;case this.InterpolantFactoryMethodSmooth:return co}}getValueSize(){return this.values.length/this.times.length}shift(e){if(e!==0){let t=this.times;for(let i=0,r=t.length;i!==r;++i)t[i]+=e}return this}scale(e){if(e!==1){let t=this.times;for(let i=0,r=t.length;i!==r;++i)t[i]*=e}return this}trim(e,t){let i=this.times,r=i.length,s=0,a=r-1;for(;s!==r&&i[s]<e;)++s;for(;a!==-1&&i[a]>t;)--a;if(++a,s!==0||a!==r){s>=a&&(a=Math.max(a,1),s=a-1);let o=this.getValueSize();this.times=i.slice(s,a),this.values=this.values.slice(s*o,a*o)}return this}validate(){let e=!0,t=this.getValueSize();t-Math.floor(t)!==0&&(console.error("THREE.KeyframeTrack: Invalid value size in track.",this),e=!1);let i=this.times,r=this.values,s=i.length;s===0&&(console.error("THREE.KeyframeTrack: Track is empty.",this),e=!1);let a=null;for(let o=0;o!==s;o++){let c=i[o];if(typeof c=="number"&&isNaN(c)){console.error("THREE.KeyframeTrack: Time is not a valid number.",this,o,c),e=!1;break}if(a!==null&&a>c){console.error("THREE.KeyframeTrack: Out of order keys.",this,o,c,a),e=!1;break}a=c}if(r!==void 0&&Ny(r))for(let o=0,c=r.length;o!==c;++o){let u=r[o];if(isNaN(u)){console.error("THREE.KeyframeTrack: Value is not a valid number.",this,o,u),e=!1;break}}return e}optimize(){let e=this.times.slice(),t=this.values.slice(),i=this.getValueSize(),r=this.getInterpolation()===co,s=e.length-1,a=1;for(let o=1;o<s;++o){let c=!1,u=e[o],l=e[o+1];if(u!==l&&(o!==1||u!==e[0]))if(r)c=!0;else{let h=o*i,f=h-i,p=h+i;for(let g=0;g!==i;++g){let _=t[h+g];if(_!==t[f+g]||_!==t[p+g]){c=!0;break}}}if(c){if(o!==a){e[a]=e[o];let h=o*i,f=a*i;for(let p=0;p!==i;++p)t[f+p]=t[h+p]}++a}}if(s>0){e[a]=e[s];for(let o=s*i,c=a*i,u=0;u!==i;++u)t[c+u]=t[o+u];++a}return a!==e.length?(this.times=e.slice(0,a),this.values=t.slice(0,a*i)):(this.times=e,this.values=t),this}clone(){let e=this.times.slice(),t=this.values.slice(),i=this.constructor,r=new i(this.name,e,t);return r.createInterpolant=this.createInterpolant,r}};xn.prototype.ValueTypeName="";xn.prototype.TimeBufferType=Float32Array;xn.prototype.ValueBufferType=Float32Array;xn.prototype.DefaultInterpolation=po;var Ri=class extends xn{constructor(e,t,i){super(e,t,i)}};Ri.prototype.ValueTypeName="bool";Ri.prototype.ValueBufferType=Array;Ri.prototype.DefaultInterpolation=Ns;Ri.prototype.InterpolantFactoryMethodLinear=void 0;Ri.prototype.InterpolantFactoryMethodSmooth=void 0;var To=class extends xn{constructor(e,t,i,r){super(e,t,i,r)}};To.prototype.ValueTypeName="color";var Ao=class extends xn{constructor(e,t,i,r){super(e,t,i,r)}};Ao.prototype.ValueTypeName="number";var Ro=class extends nr{constructor(e,t,i,r){super(e,t,i,r)}interpolate_(e,t,i,r){let s=this.resultBuffer,a=this.sampleValues,o=this.valueSize,c=(i-t)/(r-t),u=e*o;for(let l=u+o;u!==l;u+=4)an.slerpFlat(s,0,a,u-o,a,u,c);return s}},Zs=class extends xn{constructor(e,t,i,r){super(e,t,i,r)}InterpolantFactoryMethodLinear(e){return new Ro(this.times,this.values,this.getValueSize(),e)}};Zs.prototype.ValueTypeName="quaternion";Zs.prototype.InterpolantFactoryMethodSmooth=void 0;var Ci=class extends xn{constructor(e,t,i){super(e,t,i)}};Ci.prototype.ValueTypeName="string";Ci.prototype.ValueBufferType=Array;Ci.prototype.DefaultInterpolation=Ns;Ci.prototype.InterpolantFactoryMethodLinear=void 0;Ci.prototype.InterpolantFactoryMethodSmooth=void 0;var Co=class extends xn{constructor(e,t,i,r){super(e,t,i,r)}};Co.prototype.ValueTypeName="vector";var Po=class{constructor(e,t,i){let r=this,s=!1,a=0,o=0,c,u=[];this.onStart=void 0,this.onLoad=e,this.onProgress=t,this.onError=i,this.abortController=new AbortController,this.itemStart=function(l){o++,s===!1&&r.onStart!==void 0&&r.onStart(l,a,o),s=!0},this.itemEnd=function(l){a++,r.onProgress!==void 0&&r.onProgress(l,a,o),a===o&&(s=!1,r.onLoad!==void 0&&r.onLoad())},this.itemError=function(l){r.onError!==void 0&&r.onError(l)},this.resolveURL=function(l){return c?c(l):l},this.setURLModifier=function(l){return c=l,this},this.addHandler=function(l,h){return u.push(l,h),this},this.removeHandler=function(l){let h=u.indexOf(l);return h!==-1&&u.splice(h,2),this},this.getHandler=function(l){for(let h=0,f=u.length;h<f;h+=2){let p=u[h],g=u[h+1];if(p.global&&(p.lastIndex=0),p.test(l))return g}return null},this.abort=function(){return this.abortController.abort(),this.abortController=new AbortController,this}}},Mp=new Po,Io=class{constructor(e){this.manager=e!==void 0?e:Mp,this.crossOrigin="anonymous",this.withCredentials=!1,this.path="",this.resourcePath="",this.requestHeader={}}load(){}loadAsync(e,t){let i=this;return new Promise(function(r,s){i.load(e,r,t,s)})}parse(){}setCrossOrigin(e){return this.crossOrigin=e,this}setWithCredentials(e){return this.withCredentials=e,this}setPath(e){return this.path=e,this}setResourcePath(e){return this.resourcePath=e,this}setRequestHeader(e){return this.requestHeader=e,this}abort(){return this}};Io.DEFAULT_MATERIAL_NAME="__DEFAULT";var Do=class extends zs{constructor(e=-1,t=1,i=1,r=-1,s=.1,a=2e3){super(),this.isOrthographicCamera=!0,this.type="OrthographicCamera",this.zoom=1,this.view=null,this.left=e,this.right=t,this.top=i,this.bottom=r,this.near=s,this.far=a,this.updateProjectionMatrix()}copy(e,t){return super.copy(e,t),this.left=e.left,this.right=e.right,this.top=e.top,this.bottom=e.bottom,this.near=e.near,this.far=e.far,this.zoom=e.zoom,this.view=e.view===null?null:Object.assign({},e.view),this}setViewOffset(e,t,i,r,s,a){this.view===null&&(this.view={enabled:!0,fullWidth:1,fullHeight:1,offsetX:0,offsetY:0,width:1,height:1}),this.view.enabled=!0,this.view.fullWidth=e,this.view.fullHeight=t,this.view.offsetX=i,this.view.offsetY=r,this.view.width=s,this.view.height=a,this.updateProjectionMatrix()}clearViewOffset(){this.view!==null&&(this.view.enabled=!1),this.updateProjectionMatrix()}updateProjectionMatrix(){let e=(this.right-this.left)/(2*this.zoom),t=(this.top-this.bottom)/(2*this.zoom),i=(this.right+this.left)/2,r=(this.top+this.bottom)/2,s=i-e,a=i+e,o=r+t,c=r-t;if(this.view!==null&&this.view.enabled){let u=(this.right-this.left)/this.view.fullWidth/this.zoom,l=(this.top-this.bottom)/this.view.fullHeight/this.zoom;s+=u*this.view.offsetX,a=s+u*this.view.width,o-=l*this.view.offsetY,c=o-l*this.view.height}this.projectionMatrix.makeOrthographic(s,a,o,c,this.near,this.far,this.coordinateSystem,this.reversedDepth),this.projectionMatrixInverse.copy(this.projectionMatrix).invert()}toJSON(e){let t=super.toJSON(e);return t.object.zoom=this.zoom,t.object.left=this.left,t.object.right=this.right,t.object.top=this.top,t.object.bottom=this.bottom,t.object.near=this.near,t.object.far=this.far,this.view!==null&&(t.object.view=Object.assign({},this.view)),t}};var No=class extends $t{constructor(e=[]){super(),this.isArrayCamera=!0,this.isMultiViewCamera=!1,this.cameras=e}};var vu="\\[\\]\\.:\\/",Ly=new RegExp("["+vu+"]","g"),xu="[^"+vu+"]",Uy="[^"+vu.replace("\\.","")+"]",Oy=/((?:WC+[\/:])*)/.source.replace("WC",xu),Fy=/(WCOD+)?/.source.replace("WCOD",Uy),By=/(?:\.(WC+)(?:\[(.+)\])?)?/.source.replace("WC",xu),ky=/\.(WC+)(?:\[(.+)\])?/.source.replace("WC",xu),zy=new RegExp("^"+Oy+Fy+By+ky+"$"),Vy=["material","materials","bones","map"],Jl=class{constructor(e,t,i){let r=i||gt.parseTrackName(t);this._targetGroup=e,this._bindings=e.subscribe_(t,r)}getValue(e,t){this.bind();let i=this._targetGroup.nCachedObjects_,r=this._bindings[i];r!==void 0&&r.getValue(e,t)}setValue(e,t){let i=this._bindings;for(let r=this._targetGroup.nCachedObjects_,s=i.length;r!==s;++r)i[r].setValue(e,t)}bind(){let e=this._bindings;for(let t=this._targetGroup.nCachedObjects_,i=e.length;t!==i;++t)e[t].bind()}unbind(){let e=this._bindings;for(let t=this._targetGroup.nCachedObjects_,i=e.length;t!==i;++t)e[t].unbind()}},gt=class n{constructor(e,t,i){this.path=t,this.parsedPath=i||n.parseTrackName(t),this.node=n.findNode(e,this.parsedPath.nodeName),this.rootNode=e,this.getValue=this._getValue_unbound,this.setValue=this._setValue_unbound}static create(e,t,i){return e&&e.isAnimationObjectGroup?new n.Composite(e,t,i):new n(e,t,i)}static sanitizeNodeName(e){return e.replace(/\s/g,"_").replace(Ly,"")}static parseTrackName(e){let t=zy.exec(e);if(t===null)throw new Error("PropertyBinding: Cannot parse trackName: "+e);let i={nodeName:t[2],objectName:t[3],objectIndex:t[4],propertyName:t[5],propertyIndex:t[6]},r=i.nodeName&&i.nodeName.lastIndexOf(".");if(r!==void 0&&r!==-1){let s=i.nodeName.substring(r+1);Vy.indexOf(s)!==-1&&(i.nodeName=i.nodeName.substring(0,r),i.objectName=s)}if(i.propertyName===null||i.propertyName.length===0)throw new Error("PropertyBinding: can not parse propertyName from trackName: "+e);return i}static findNode(e,t){if(t===void 0||t===""||t==="."||t===-1||t===e.name||t===e.uuid)return e;if(e.skeleton){let i=e.skeleton.getBoneByName(t);if(i!==void 0)return i}if(e.children){let i=function(s){for(let a=0;a<s.length;a++){let o=s[a];if(o.name===t||o.uuid===t)return o;let c=i(o.children);if(c)return c}return null},r=i(e.children);if(r)return r}return null}_getValue_unavailable(){}_setValue_unavailable(){}_getValue_direct(e,t){e[t]=this.targetObject[this.propertyName]}_getValue_array(e,t){let i=this.resolvedProperty;for(let r=0,s=i.length;r!==s;++r)e[t++]=i[r]}_getValue_arrayElement(e,t){e[t]=this.resolvedProperty[this.propertyIndex]}_getValue_toArray(e,t){this.resolvedProperty.toArray(e,t)}_setValue_direct(e,t){this.targetObject[this.propertyName]=e[t]}_setValue_direct_setNeedsUpdate(e,t){this.targetObject[this.propertyName]=e[t],this.targetObject.needsUpdate=!0}_setValue_direct_setMatrixWorldNeedsUpdate(e,t){this.targetObject[this.propertyName]=e[t],this.targetObject.matrixWorldNeedsUpdate=!0}_setValue_array(e,t){let i=this.resolvedProperty;for(let r=0,s=i.length;r!==s;++r)i[r]=e[t++]}_setValue_array_setNeedsUpdate(e,t){let i=this.resolvedProperty;for(let r=0,s=i.length;r!==s;++r)i[r]=e[t++];this.targetObject.needsUpdate=!0}_setValue_array_setMatrixWorldNeedsUpdate(e,t){let i=this.resolvedProperty;for(let r=0,s=i.length;r!==s;++r)i[r]=e[t++];this.targetObject.matrixWorldNeedsUpdate=!0}_setValue_arrayElement(e,t){this.resolvedProperty[this.propertyIndex]=e[t]}_setValue_arrayElement_setNeedsUpdate(e,t){this.resolvedProperty[this.propertyIndex]=e[t],this.targetObject.needsUpdate=!0}_setValue_arrayElement_setMatrixWorldNeedsUpdate(e,t){this.resolvedProperty[this.propertyIndex]=e[t],this.targetObject.matrixWorldNeedsUpdate=!0}_setValue_fromArray(e,t){this.resolvedProperty.fromArray(e,t)}_setValue_fromArray_setNeedsUpdate(e,t){this.resolvedProperty.fromArray(e,t),this.targetObject.needsUpdate=!0}_setValue_fromArray_setMatrixWorldNeedsUpdate(e,t){this.resolvedProperty.fromArray(e,t),this.targetObject.matrixWorldNeedsUpdate=!0}_getValue_unbound(e,t){this.bind(),this.getValue(e,t)}_setValue_unbound(e,t){this.bind(),this.setValue(e,t)}bind(){let e=this.node,t=this.parsedPath,i=t.objectName,r=t.propertyName,s=t.propertyIndex;if(e||(e=n.findNode(this.rootNode,t.nodeName),this.node=e),this.getValue=this._getValue_unavailable,this.setValue=this._setValue_unavailable,!e){console.warn("THREE.PropertyBinding: No target node found for track: "+this.path+".");return}if(i){let u=t.objectIndex;switch(i){case"materials":if(!e.material){console.error("THREE.PropertyBinding: Can not bind to material as node does not have a material.",this);return}if(!e.material.materials){console.error("THREE.PropertyBinding: Can not bind to material.materials as node.material does not have a materials array.",this);return}e=e.material.materials;break;case"bones":if(!e.skeleton){console.error("THREE.PropertyBinding: Can not bind to bones as node does not have a skeleton.",this);return}e=e.skeleton.bones;for(let l=0;l<e.length;l++)if(e[l].name===u){u=l;break}break;case"map":if("map"in e){e=e.map;break}if(!e.material){console.error("THREE.PropertyBinding: Can not bind to material as node does not have a material.",this);return}if(!e.material.map){console.error("THREE.PropertyBinding: Can not bind to material.map as node.material does not have a map.",this);return}e=e.material.map;break;default:if(e[i]===void 0){console.error("THREE.PropertyBinding: Can not bind to objectName of node undefined.",this);return}e=e[i]}if(u!==void 0){if(e[u]===void 0){console.error("THREE.PropertyBinding: Trying to bind to objectIndex of objectName, but is undefined.",this,e);return}e=e[u]}}let a=e[r];if(a===void 0){let u=t.nodeName;console.error("THREE.PropertyBinding: Trying to update property for track: "+u+"."+r+" but it wasn't found.",e);return}let o=this.Versioning.None;this.targetObject=e,e.isMaterial===!0?o=this.Versioning.NeedsUpdate:e.isObject3D===!0&&(o=this.Versioning.MatrixWorldNeedsUpdate);let c=this.BindingType.Direct;if(s!==void 0){if(r==="morphTargetInfluences"){if(!e.geometry){console.error("THREE.PropertyBinding: Can not bind to morphTargetInfluences because node does not have a geometry.",this);return}if(!e.geometry.morphAttributes){console.error("THREE.PropertyBinding: Can not bind to morphTargetInfluences because node does not have a geometry.morphAttributes.",this);return}e.morphTargetDictionary[s]!==void 0&&(s=e.morphTargetDictionary[s])}c=this.BindingType.ArrayElement,this.resolvedProperty=a,this.propertyIndex=s}else a.fromArray!==void 0&&a.toArray!==void 0?(c=this.BindingType.HasFromToArray,this.resolvedProperty=a):Array.isArray(a)?(c=this.BindingType.EntireArray,this.resolvedProperty=a):this.propertyName=r;this.getValue=this.GetterByBindingType[c],this.setValue=this.SetterByBindingTypeAndVersioning[c][o]}unbind(){this.node=null,this.getValue=this._getValue_unbound,this.setValue=this._setValue_unbound}};gt.Composite=Jl;gt.prototype.BindingType={Direct:0,EntireArray:1,ArrayElement:2,HasFromToArray:3};gt.prototype.Versioning={None:0,NeedsUpdate:1,MatrixWorldNeedsUpdate:2};gt.prototype.GetterByBindingType=[gt.prototype._getValue_direct,gt.prototype._getValue_array,gt.prototype._getValue_arrayElement,gt.prototype._getValue_toArray];gt.prototype.SetterByBindingTypeAndVersioning=[[gt.prototype._setValue_direct,gt.prototype._setValue_direct_setNeedsUpdate,gt.prototype._setValue_direct_setMatrixWorldNeedsUpdate],[gt.prototype._setValue_array,gt.prototype._setValue_array_setNeedsUpdate,gt.prototype._setValue_array_setMatrixWorldNeedsUpdate],[gt.prototype._setValue_arrayElement,gt.prototype._setValue_arrayElement_setNeedsUpdate,gt.prototype._setValue_arrayElement_setMatrixWorldNeedsUpdate],[gt.prototype._setValue_fromArray,gt.prototype._setValue_fromArray_setNeedsUpdate,gt.prototype._setValue_fromArray_setMatrixWorldNeedsUpdate]];var NC=new Float32Array(1);var Cf=new _t,Ks=class{constructor(e,t,i=0,r=1/0){this.ray=new er(e,t),this.near=i,this.far=r,this.camera=null,this.layers=new jr,this.params={Mesh:{},Line:{threshold:1},LOD:{},Points:{threshold:1},Sprite:{}}}set(e,t){this.ray.set(e,t)}setFromCamera(e,t){t.isPerspectiveCamera?(this.ray.origin.setFromMatrixPosition(t.matrixWorld),this.ray.direction.set(e.x,e.y,.5).unproject(t).sub(this.ray.origin).normalize(),this.camera=t):t.isOrthographicCamera?(this.ray.origin.set(e.x,e.y,(t.near+t.far)/(t.near-t.far)).unproject(t),this.ray.direction.set(0,0,-1).transformDirection(t.matrixWorld),this.camera=t):console.error("THREE.Raycaster: Unsupported camera type: "+t.type)}setFromXRController(e){return Cf.identity().extractRotation(e.matrixWorld),this.ray.origin.setFromMatrixPosition(e.matrixWorld),this.ray.direction.set(0,0,-1).applyMatrix4(Cf),this}intersectObject(e,t=!0,i=[]){return Ql(e,this,i,t),i.sort(Pf),i}intersectObjects(e,t=!0,i=[]){for(let r=0,s=e.length;r<s;r++)Ql(e[r],this,i,t);return i.sort(Pf),i}};function Pf(n,e){return n.distance-e.distance}function Ql(n,e,t,i){let r=!0;if(n.layers.test(e.layers)&&n.raycast(e,t)===!1&&(r=!1),r===!0&&i===!0){let s=n.children;for(let a=0,o=s.length;a<o;a++)Ql(s[a],e,t,!0)}}var Kr=class{constructor(e=1,t=0,i=0){this.radius=e,this.phi=t,this.theta=i}set(e,t,i){return this.radius=e,this.phi=t,this.theta=i,this}copy(e){return this.radius=e.radius,this.phi=e.phi,this.theta=e.theta,this}makeSafe(){return this.phi=We(this.phi,1e-6,Math.PI-1e-6),this}setFromVector3(e){return this.setFromCartesianCoords(e.x,e.y,e.z)}setFromCartesianCoords(e,t,i){return this.radius=Math.sqrt(e*e+t*t+i*i),this.radius===0?(this.theta=0,this.phi=0):(this.theta=Math.atan2(e,i),this.phi=Math.acos(We(t/this.radius,-1,1))),this}clone(){return new this.constructor().copy(this)}};var Js=class extends Gn{constructor(e,t=null){super(),this.object=e,this.domElement=t,this.enabled=!0,this.state=-1,this.keys={},this.mouseButtons={LEFT:null,MIDDLE:null,RIGHT:null},this.touches={ONE:null,TWO:null}}connect(e){if(e===void 0){console.warn("THREE.Controls: connect() now requires an element.");return}this.domElement!==null&&this.disconnect(),this.domElement=e}disconnect(){}dispose(){}update(){}};function Su(n,e,t,i){let r=$y(i);switch(t){case hu:return n*e;case Xo:return n*e/r.components*r.byteLength;case jo:return n*e/r.components*r.byteLength;case fu:return n*e*2/r.components*r.byteLength;case Yo:return n*e*2/r.components*r.byteLength;case du:return n*e*3/r.components*r.byteLength;case Tn:return n*e*4/r.components*r.byteLength;case Zo:return n*e*4/r.components*r.byteLength;case ta:case na:return Math.floor((n+3)/4)*Math.floor((e+3)/4)*8;case ia:case ra:return Math.floor((n+3)/4)*Math.floor((e+3)/4)*16;case Jo:case ec:return Math.max(n,16)*Math.max(e,8)/4;case Ko:case Qo:return Math.max(n,8)*Math.max(e,8)/2;case tc:case nc:return Math.floor((n+3)/4)*Math.floor((e+3)/4)*8;case ic:return Math.floor((n+3)/4)*Math.floor((e+3)/4)*16;case rc:return Math.floor((n+3)/4)*Math.floor((e+3)/4)*16;case sc:return Math.floor((n+4)/5)*Math.floor((e+3)/4)*16;case ac:return Math.floor((n+4)/5)*Math.floor((e+4)/5)*16;case oc:return Math.floor((n+5)/6)*Math.floor((e+4)/5)*16;case cc:return Math.floor((n+5)/6)*Math.floor((e+5)/6)*16;case lc:return Math.floor((n+7)/8)*Math.floor((e+4)/5)*16;case uc:return Math.floor((n+7)/8)*Math.floor((e+5)/6)*16;case hc:return Math.floor((n+7)/8)*Math.floor((e+7)/8)*16;case dc:return Math.floor((n+9)/10)*Math.floor((e+4)/5)*16;case fc:return Math.floor((n+9)/10)*Math.floor((e+5)/6)*16;case pc:return Math.floor((n+9)/10)*Math.floor((e+7)/8)*16;case mc:return Math.floor((n+9)/10)*Math.floor((e+9)/10)*16;case gc:return Math.floor((n+11)/12)*Math.floor((e+9)/10)*16;case _c:return Math.floor((n+11)/12)*Math.floor((e+11)/12)*16;case yc:case vc:case xc:return Math.ceil(n/4)*Math.ceil(e/4)*16;case Sc:case bc:return Math.ceil(n/4)*Math.ceil(e/4)*8;case Mc:case Ec:return Math.ceil(n/4)*Math.ceil(e/4)*16}throw new Error(`Unable to determine texture byte length for ${t} format.`)}function $y(n){switch(n){case Zn:case ou:return{byteLength:1,components:1};case Jr:case cu:case Qr:return{byteLength:2,components:1};case Wo:case qo:return{byteLength:2,components:4};case Ni:case Go:case Bn:return{byteLength:4,components:1};case lu:case uu:return{byteLength:4,components:3}}throw new Error(`Unknown texture type ${n}.`)}typeof __THREE_DEVTOOLS__<"u"&&__THREE_DEVTOOLS__.dispatchEvent(new CustomEvent("register",{detail:{revision:"180"}}));typeof window<"u"&&(window.__THREE__?console.warn("WARNING: Multiple instances of Three.js being imported."):window.__THREE__="180");function Xp(){let n=null,e=!1,t=null,i=null;function r(s,a){t(s,a),i=n.requestAnimationFrame(r)}return{start:function(){e!==!0&&t!==null&&(i=n.requestAnimationFrame(r),e=!0)},stop:function(){n.cancelAnimationFrame(i),e=!1},setAnimationLoop:function(s){t=s},setContext:function(s){n=s}}}function Gy(n){let e=new WeakMap;function t(o,c){let u=o.array,l=o.usage,h=u.byteLength,f=n.createBuffer();n.bindBuffer(c,f),n.bufferData(c,u,l),o.onUploadCallback();let p;if(u instanceof Float32Array)p=n.FLOAT;else if(typeof Float16Array<"u"&&u instanceof Float16Array)p=n.HALF_FLOAT;else if(u instanceof Uint16Array)o.isFloat16BufferAttribute?p=n.HALF_FLOAT:p=n.UNSIGNED_SHORT;else if(u instanceof Int16Array)p=n.SHORT;else if(u instanceof Uint32Array)p=n.UNSIGNED_INT;else if(u instanceof Int32Array)p=n.INT;else if(u instanceof Int8Array)p=n.BYTE;else if(u instanceof Uint8Array)p=n.UNSIGNED_BYTE;else if(u instanceof Uint8ClampedArray)p=n.UNSIGNED_BYTE;else throw new Error("THREE.WebGLAttributes: Unsupported buffer data format: "+u);return{buffer:f,type:p,bytesPerElement:u.BYTES_PER_ELEMENT,version:o.version,size:h}}function i(o,c,u){let l=c.array,h=c.updateRanges;if(n.bindBuffer(u,o),h.length===0)n.bufferSubData(u,0,l);else{h.sort((p,g)=>p.start-g.start);let f=0;for(let p=1;p<h.length;p++){let g=h[f],_=h[p];_.start<=g.start+g.count+1?g.count=Math.max(g.count,_.start+_.count-g.start):(++f,h[f]=_)}h.length=f+1;for(let p=0,g=h.length;p<g;p++){let _=h[p];n.bufferSubData(u,_.start*l.BYTES_PER_ELEMENT,l,_.start,_.count)}c.clearUpdateRanges()}c.onUploadCallback()}function r(o){return o.isInterleavedBufferAttribute&&(o=o.data),e.get(o)}function s(o){o.isInterleavedBufferAttribute&&(o=o.data);let c=e.get(o);c&&(n.deleteBuffer(c.buffer),e.delete(o))}function a(o,c){if(o.isInterleavedBufferAttribute&&(o=o.data),o.isGLBufferAttribute){let l=e.get(o);(!l||l.version<o.version)&&e.set(o,{buffer:o.buffer,type:o.type,bytesPerElement:o.elementSize,version:o.version});return}let u=e.get(o);if(u===void 0)e.set(o,t(o,c));else if(u.version<o.version){if(u.size!==o.array.byteLength)throw new Error("THREE.WebGLAttributes: The size of the buffer attribute's array buffer does not match the original size. Resizing buffer attributes is not supported.");i(u.buffer,o,c),u.version=o.version}}return{get:r,remove:s,update:a}}var Wy=`#ifdef USE_ALPHAHASH
	if ( diffuseColor.a < getAlphaHashThreshold( vPosition ) ) discard;
#endif`,qy=`#ifdef USE_ALPHAHASH
	const float ALPHA_HASH_SCALE = 0.05;
	float hash2D( vec2 value ) {
		return fract( 1.0e4 * sin( 17.0 * value.x + 0.1 * value.y ) * ( 0.1 + abs( sin( 13.0 * value.y + value.x ) ) ) );
	}
	float hash3D( vec3 value ) {
		return hash2D( vec2( hash2D( value.xy ), value.z ) );
	}
	float getAlphaHashThreshold( vec3 position ) {
		float maxDeriv = max(
			length( dFdx( position.xyz ) ),
			length( dFdy( position.xyz ) )
		);
		float pixScale = 1.0 / ( ALPHA_HASH_SCALE * maxDeriv );
		vec2 pixScales = vec2(
			exp2( floor( log2( pixScale ) ) ),
			exp2( ceil( log2( pixScale ) ) )
		);
		vec2 alpha = vec2(
			hash3D( floor( pixScales.x * position.xyz ) ),
			hash3D( floor( pixScales.y * position.xyz ) )
		);
		float lerpFactor = fract( log2( pixScale ) );
		float x = ( 1.0 - lerpFactor ) * alpha.x + lerpFactor * alpha.y;
		float a = min( lerpFactor, 1.0 - lerpFactor );
		vec3 cases = vec3(
			x * x / ( 2.0 * a * ( 1.0 - a ) ),
			( x - 0.5 * a ) / ( 1.0 - a ),
			1.0 - ( ( 1.0 - x ) * ( 1.0 - x ) / ( 2.0 * a * ( 1.0 - a ) ) )
		);
		float threshold = ( x < ( 1.0 - a ) )
			? ( ( x < a ) ? cases.x : cases.y )
			: cases.z;
		return clamp( threshold , 1.0e-6, 1.0 );
	}
#endif`,Xy=`#ifdef USE_ALPHAMAP
	diffuseColor.a *= texture2D( alphaMap, vAlphaMapUv ).g;
#endif`,jy=`#ifdef USE_ALPHAMAP
	uniform sampler2D alphaMap;
#endif`,Yy=`#ifdef USE_ALPHATEST
	#ifdef ALPHA_TO_COVERAGE
	diffuseColor.a = smoothstep( alphaTest, alphaTest + fwidth( diffuseColor.a ), diffuseColor.a );
	if ( diffuseColor.a == 0.0 ) discard;
	#else
	if ( diffuseColor.a < alphaTest ) discard;
	#endif
#endif`,Zy=`#ifdef USE_ALPHATEST
	uniform float alphaTest;
#endif`,Ky=`#ifdef USE_AOMAP
	float ambientOcclusion = ( texture2D( aoMap, vAoMapUv ).r - 1.0 ) * aoMapIntensity + 1.0;
	reflectedLight.indirectDiffuse *= ambientOcclusion;
	#if defined( USE_CLEARCOAT ) 
		clearcoatSpecularIndirect *= ambientOcclusion;
	#endif
	#if defined( USE_SHEEN ) 
		sheenSpecularIndirect *= ambientOcclusion;
	#endif
	#if defined( USE_ENVMAP ) && defined( STANDARD )
		float dotNV = saturate( dot( geometryNormal, geometryViewDir ) );
		reflectedLight.indirectSpecular *= computeSpecularOcclusion( dotNV, ambientOcclusion, material.roughness );
	#endif
#endif`,Jy=`#ifdef USE_AOMAP
	uniform sampler2D aoMap;
	uniform float aoMapIntensity;
#endif`,Qy=`#ifdef USE_BATCHING
	#if ! defined( GL_ANGLE_multi_draw )
	#define gl_DrawID _gl_DrawID
	uniform int _gl_DrawID;
	#endif
	uniform highp sampler2D batchingTexture;
	uniform highp usampler2D batchingIdTexture;
	mat4 getBatchingMatrix( const in float i ) {
		int size = textureSize( batchingTexture, 0 ).x;
		int j = int( i ) * 4;
		int x = j % size;
		int y = j / size;
		vec4 v1 = texelFetch( batchingTexture, ivec2( x, y ), 0 );
		vec4 v2 = texelFetch( batchingTexture, ivec2( x + 1, y ), 0 );
		vec4 v3 = texelFetch( batchingTexture, ivec2( x + 2, y ), 0 );
		vec4 v4 = texelFetch( batchingTexture, ivec2( x + 3, y ), 0 );
		return mat4( v1, v2, v3, v4 );
	}
	float getIndirectIndex( const in int i ) {
		int size = textureSize( batchingIdTexture, 0 ).x;
		int x = i % size;
		int y = i / size;
		return float( texelFetch( batchingIdTexture, ivec2( x, y ), 0 ).r );
	}
#endif
#ifdef USE_BATCHING_COLOR
	uniform sampler2D batchingColorTexture;
	vec3 getBatchingColor( const in float i ) {
		int size = textureSize( batchingColorTexture, 0 ).x;
		int j = int( i );
		int x = j % size;
		int y = j / size;
		return texelFetch( batchingColorTexture, ivec2( x, y ), 0 ).rgb;
	}
#endif`,ev=`#ifdef USE_BATCHING
	mat4 batchingMatrix = getBatchingMatrix( getIndirectIndex( gl_DrawID ) );
#endif`,tv=`vec3 transformed = vec3( position );
#ifdef USE_ALPHAHASH
	vPosition = vec3( position );
#endif`,nv=`vec3 objectNormal = vec3( normal );
#ifdef USE_TANGENT
	vec3 objectTangent = vec3( tangent.xyz );
#endif`,iv=`float G_BlinnPhong_Implicit( ) {
	return 0.25;
}
float D_BlinnPhong( const in float shininess, const in float dotNH ) {
	return RECIPROCAL_PI * ( shininess * 0.5 + 1.0 ) * pow( dotNH, shininess );
}
vec3 BRDF_BlinnPhong( const in vec3 lightDir, const in vec3 viewDir, const in vec3 normal, const in vec3 specularColor, const in float shininess ) {
	vec3 halfDir = normalize( lightDir + viewDir );
	float dotNH = saturate( dot( normal, halfDir ) );
	float dotVH = saturate( dot( viewDir, halfDir ) );
	vec3 F = F_Schlick( specularColor, 1.0, dotVH );
	float G = G_BlinnPhong_Implicit( );
	float D = D_BlinnPhong( shininess, dotNH );
	return F * ( G * D );
} // validated`,rv=`#ifdef USE_IRIDESCENCE
	const mat3 XYZ_TO_REC709 = mat3(
		 3.2404542, -0.9692660,  0.0556434,
		-1.5371385,  1.8760108, -0.2040259,
		-0.4985314,  0.0415560,  1.0572252
	);
	vec3 Fresnel0ToIor( vec3 fresnel0 ) {
		vec3 sqrtF0 = sqrt( fresnel0 );
		return ( vec3( 1.0 ) + sqrtF0 ) / ( vec3( 1.0 ) - sqrtF0 );
	}
	vec3 IorToFresnel0( vec3 transmittedIor, float incidentIor ) {
		return pow2( ( transmittedIor - vec3( incidentIor ) ) / ( transmittedIor + vec3( incidentIor ) ) );
	}
	float IorToFresnel0( float transmittedIor, float incidentIor ) {
		return pow2( ( transmittedIor - incidentIor ) / ( transmittedIor + incidentIor ));
	}
	vec3 evalSensitivity( float OPD, vec3 shift ) {
		float phase = 2.0 * PI * OPD * 1.0e-9;
		vec3 val = vec3( 5.4856e-13, 4.4201e-13, 5.2481e-13 );
		vec3 pos = vec3( 1.6810e+06, 1.7953e+06, 2.2084e+06 );
		vec3 var = vec3( 4.3278e+09, 9.3046e+09, 6.6121e+09 );
		vec3 xyz = val * sqrt( 2.0 * PI * var ) * cos( pos * phase + shift ) * exp( - pow2( phase ) * var );
		xyz.x += 9.7470e-14 * sqrt( 2.0 * PI * 4.5282e+09 ) * cos( 2.2399e+06 * phase + shift[ 0 ] ) * exp( - 4.5282e+09 * pow2( phase ) );
		xyz /= 1.0685e-7;
		vec3 rgb = XYZ_TO_REC709 * xyz;
		return rgb;
	}
	vec3 evalIridescence( float outsideIOR, float eta2, float cosTheta1, float thinFilmThickness, vec3 baseF0 ) {
		vec3 I;
		float iridescenceIOR = mix( outsideIOR, eta2, smoothstep( 0.0, 0.03, thinFilmThickness ) );
		float sinTheta2Sq = pow2( outsideIOR / iridescenceIOR ) * ( 1.0 - pow2( cosTheta1 ) );
		float cosTheta2Sq = 1.0 - sinTheta2Sq;
		if ( cosTheta2Sq < 0.0 ) {
			return vec3( 1.0 );
		}
		float cosTheta2 = sqrt( cosTheta2Sq );
		float R0 = IorToFresnel0( iridescenceIOR, outsideIOR );
		float R12 = F_Schlick( R0, 1.0, cosTheta1 );
		float T121 = 1.0 - R12;
		float phi12 = 0.0;
		if ( iridescenceIOR < outsideIOR ) phi12 = PI;
		float phi21 = PI - phi12;
		vec3 baseIOR = Fresnel0ToIor( clamp( baseF0, 0.0, 0.9999 ) );		vec3 R1 = IorToFresnel0( baseIOR, iridescenceIOR );
		vec3 R23 = F_Schlick( R1, 1.0, cosTheta2 );
		vec3 phi23 = vec3( 0.0 );
		if ( baseIOR[ 0 ] < iridescenceIOR ) phi23[ 0 ] = PI;
		if ( baseIOR[ 1 ] < iridescenceIOR ) phi23[ 1 ] = PI;
		if ( baseIOR[ 2 ] < iridescenceIOR ) phi23[ 2 ] = PI;
		float OPD = 2.0 * iridescenceIOR * thinFilmThickness * cosTheta2;
		vec3 phi = vec3( phi21 ) + phi23;
		vec3 R123 = clamp( R12 * R23, 1e-5, 0.9999 );
		vec3 r123 = sqrt( R123 );
		vec3 Rs = pow2( T121 ) * R23 / ( vec3( 1.0 ) - R123 );
		vec3 C0 = R12 + Rs;
		I = C0;
		vec3 Cm = Rs - T121;
		for ( int m = 1; m <= 2; ++ m ) {
			Cm *= r123;
			vec3 Sm = 2.0 * evalSensitivity( float( m ) * OPD, float( m ) * phi );
			I += Cm * Sm;
		}
		return max( I, vec3( 0.0 ) );
	}
#endif`,sv=`#ifdef USE_BUMPMAP
	uniform sampler2D bumpMap;
	uniform float bumpScale;
	vec2 dHdxy_fwd() {
		vec2 dSTdx = dFdx( vBumpMapUv );
		vec2 dSTdy = dFdy( vBumpMapUv );
		float Hll = bumpScale * texture2D( bumpMap, vBumpMapUv ).x;
		float dBx = bumpScale * texture2D( bumpMap, vBumpMapUv + dSTdx ).x - Hll;
		float dBy = bumpScale * texture2D( bumpMap, vBumpMapUv + dSTdy ).x - Hll;
		return vec2( dBx, dBy );
	}
	vec3 perturbNormalArb( vec3 surf_pos, vec3 surf_norm, vec2 dHdxy, float faceDirection ) {
		vec3 vSigmaX = normalize( dFdx( surf_pos.xyz ) );
		vec3 vSigmaY = normalize( dFdy( surf_pos.xyz ) );
		vec3 vN = surf_norm;
		vec3 R1 = cross( vSigmaY, vN );
		vec3 R2 = cross( vN, vSigmaX );
		float fDet = dot( vSigmaX, R1 ) * faceDirection;
		vec3 vGrad = sign( fDet ) * ( dHdxy.x * R1 + dHdxy.y * R2 );
		return normalize( abs( fDet ) * surf_norm - vGrad );
	}
#endif`,av=`#if NUM_CLIPPING_PLANES > 0
	vec4 plane;
	#ifdef ALPHA_TO_COVERAGE
		float distanceToPlane, distanceGradient;
		float clipOpacity = 1.0;
		#pragma unroll_loop_start
		for ( int i = 0; i < UNION_CLIPPING_PLANES; i ++ ) {
			plane = clippingPlanes[ i ];
			distanceToPlane = - dot( vClipPosition, plane.xyz ) + plane.w;
			distanceGradient = fwidth( distanceToPlane ) / 2.0;
			clipOpacity *= smoothstep( - distanceGradient, distanceGradient, distanceToPlane );
			if ( clipOpacity == 0.0 ) discard;
		}
		#pragma unroll_loop_end
		#if UNION_CLIPPING_PLANES < NUM_CLIPPING_PLANES
			float unionClipOpacity = 1.0;
			#pragma unroll_loop_start
			for ( int i = UNION_CLIPPING_PLANES; i < NUM_CLIPPING_PLANES; i ++ ) {
				plane = clippingPlanes[ i ];
				distanceToPlane = - dot( vClipPosition, plane.xyz ) + plane.w;
				distanceGradient = fwidth( distanceToPlane ) / 2.0;
				unionClipOpacity *= 1.0 - smoothstep( - distanceGradient, distanceGradient, distanceToPlane );
			}
			#pragma unroll_loop_end
			clipOpacity *= 1.0 - unionClipOpacity;
		#endif
		diffuseColor.a *= clipOpacity;
		if ( diffuseColor.a == 0.0 ) discard;
	#else
		#pragma unroll_loop_start
		for ( int i = 0; i < UNION_CLIPPING_PLANES; i ++ ) {
			plane = clippingPlanes[ i ];
			if ( dot( vClipPosition, plane.xyz ) > plane.w ) discard;
		}
		#pragma unroll_loop_end
		#if UNION_CLIPPING_PLANES < NUM_CLIPPING_PLANES
			bool clipped = true;
			#pragma unroll_loop_start
			for ( int i = UNION_CLIPPING_PLANES; i < NUM_CLIPPING_PLANES; i ++ ) {
				plane = clippingPlanes[ i ];
				clipped = ( dot( vClipPosition, plane.xyz ) > plane.w ) && clipped;
			}
			#pragma unroll_loop_end
			if ( clipped ) discard;
		#endif
	#endif
#endif`,ov=`#if NUM_CLIPPING_PLANES > 0
	varying vec3 vClipPosition;
	uniform vec4 clippingPlanes[ NUM_CLIPPING_PLANES ];
#endif`,cv=`#if NUM_CLIPPING_PLANES > 0
	varying vec3 vClipPosition;
#endif`,lv=`#if NUM_CLIPPING_PLANES > 0
	vClipPosition = - mvPosition.xyz;
#endif`,uv=`#if defined( USE_COLOR_ALPHA )
	diffuseColor *= vColor;
#elif defined( USE_COLOR )
	diffuseColor.rgb *= vColor;
#endif`,hv=`#if defined( USE_COLOR_ALPHA )
	varying vec4 vColor;
#elif defined( USE_COLOR )
	varying vec3 vColor;
#endif`,dv=`#if defined( USE_COLOR_ALPHA )
	varying vec4 vColor;
#elif defined( USE_COLOR ) || defined( USE_INSTANCING_COLOR ) || defined( USE_BATCHING_COLOR )
	varying vec3 vColor;
#endif`,fv=`#if defined( USE_COLOR_ALPHA )
	vColor = vec4( 1.0 );
#elif defined( USE_COLOR ) || defined( USE_INSTANCING_COLOR ) || defined( USE_BATCHING_COLOR )
	vColor = vec3( 1.0 );
#endif
#ifdef USE_COLOR
	vColor *= color;
#endif
#ifdef USE_INSTANCING_COLOR
	vColor.xyz *= instanceColor.xyz;
#endif
#ifdef USE_BATCHING_COLOR
	vec3 batchingColor = getBatchingColor( getIndirectIndex( gl_DrawID ) );
	vColor.xyz *= batchingColor.xyz;
#endif`,pv=`#define PI 3.141592653589793
#define PI2 6.283185307179586
#define PI_HALF 1.5707963267948966
#define RECIPROCAL_PI 0.3183098861837907
#define RECIPROCAL_PI2 0.15915494309189535
#define EPSILON 1e-6
#ifndef saturate
#define saturate( a ) clamp( a, 0.0, 1.0 )
#endif
#define whiteComplement( a ) ( 1.0 - saturate( a ) )
float pow2( const in float x ) { return x*x; }
vec3 pow2( const in vec3 x ) { return x*x; }
float pow3( const in float x ) { return x*x*x; }
float pow4( const in float x ) { float x2 = x*x; return x2*x2; }
float max3( const in vec3 v ) { return max( max( v.x, v.y ), v.z ); }
float average( const in vec3 v ) { return dot( v, vec3( 0.3333333 ) ); }
highp float rand( const in vec2 uv ) {
	const highp float a = 12.9898, b = 78.233, c = 43758.5453;
	highp float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
	return fract( sin( sn ) * c );
}
#ifdef HIGH_PRECISION
	float precisionSafeLength( vec3 v ) { return length( v ); }
#else
	float precisionSafeLength( vec3 v ) {
		float maxComponent = max3( abs( v ) );
		return length( v / maxComponent ) * maxComponent;
	}
#endif
struct IncidentLight {
	vec3 color;
	vec3 direction;
	bool visible;
};
struct ReflectedLight {
	vec3 directDiffuse;
	vec3 directSpecular;
	vec3 indirectDiffuse;
	vec3 indirectSpecular;
};
#ifdef USE_ALPHAHASH
	varying vec3 vPosition;
#endif
vec3 transformDirection( in vec3 dir, in mat4 matrix ) {
	return normalize( ( matrix * vec4( dir, 0.0 ) ).xyz );
}
vec3 inverseTransformDirection( in vec3 dir, in mat4 matrix ) {
	return normalize( ( vec4( dir, 0.0 ) * matrix ).xyz );
}
mat3 transposeMat3( const in mat3 m ) {
	mat3 tmp;
	tmp[ 0 ] = vec3( m[ 0 ].x, m[ 1 ].x, m[ 2 ].x );
	tmp[ 1 ] = vec3( m[ 0 ].y, m[ 1 ].y, m[ 2 ].y );
	tmp[ 2 ] = vec3( m[ 0 ].z, m[ 1 ].z, m[ 2 ].z );
	return tmp;
}
bool isPerspectiveMatrix( mat4 m ) {
	return m[ 2 ][ 3 ] == - 1.0;
}
vec2 equirectUv( in vec3 dir ) {
	float u = atan( dir.z, dir.x ) * RECIPROCAL_PI2 + 0.5;
	float v = asin( clamp( dir.y, - 1.0, 1.0 ) ) * RECIPROCAL_PI + 0.5;
	return vec2( u, v );
}
vec3 BRDF_Lambert( const in vec3 diffuseColor ) {
	return RECIPROCAL_PI * diffuseColor;
}
vec3 F_Schlick( const in vec3 f0, const in float f90, const in float dotVH ) {
	float fresnel = exp2( ( - 5.55473 * dotVH - 6.98316 ) * dotVH );
	return f0 * ( 1.0 - fresnel ) + ( f90 * fresnel );
}
float F_Schlick( const in float f0, const in float f90, const in float dotVH ) {
	float fresnel = exp2( ( - 5.55473 * dotVH - 6.98316 ) * dotVH );
	return f0 * ( 1.0 - fresnel ) + ( f90 * fresnel );
} // validated`,mv=`#ifdef ENVMAP_TYPE_CUBE_UV
	#define cubeUV_minMipLevel 4.0
	#define cubeUV_minTileSize 16.0
	float getFace( vec3 direction ) {
		vec3 absDirection = abs( direction );
		float face = - 1.0;
		if ( absDirection.x > absDirection.z ) {
			if ( absDirection.x > absDirection.y )
				face = direction.x > 0.0 ? 0.0 : 3.0;
			else
				face = direction.y > 0.0 ? 1.0 : 4.0;
		} else {
			if ( absDirection.z > absDirection.y )
				face = direction.z > 0.0 ? 2.0 : 5.0;
			else
				face = direction.y > 0.0 ? 1.0 : 4.0;
		}
		return face;
	}
	vec2 getUV( vec3 direction, float face ) {
		vec2 uv;
		if ( face == 0.0 ) {
			uv = vec2( direction.z, direction.y ) / abs( direction.x );
		} else if ( face == 1.0 ) {
			uv = vec2( - direction.x, - direction.z ) / abs( direction.y );
		} else if ( face == 2.0 ) {
			uv = vec2( - direction.x, direction.y ) / abs( direction.z );
		} else if ( face == 3.0 ) {
			uv = vec2( - direction.z, direction.y ) / abs( direction.x );
		} else if ( face == 4.0 ) {
			uv = vec2( - direction.x, direction.z ) / abs( direction.y );
		} else {
			uv = vec2( direction.x, direction.y ) / abs( direction.z );
		}
		return 0.5 * ( uv + 1.0 );
	}
	vec3 bilinearCubeUV( sampler2D envMap, vec3 direction, float mipInt ) {
		float face = getFace( direction );
		float filterInt = max( cubeUV_minMipLevel - mipInt, 0.0 );
		mipInt = max( mipInt, cubeUV_minMipLevel );
		float faceSize = exp2( mipInt );
		highp vec2 uv = getUV( direction, face ) * ( faceSize - 2.0 ) + 1.0;
		if ( face > 2.0 ) {
			uv.y += faceSize;
			face -= 3.0;
		}
		uv.x += face * faceSize;
		uv.x += filterInt * 3.0 * cubeUV_minTileSize;
		uv.y += 4.0 * ( exp2( CUBEUV_MAX_MIP ) - faceSize );
		uv.x *= CUBEUV_TEXEL_WIDTH;
		uv.y *= CUBEUV_TEXEL_HEIGHT;
		#ifdef texture2DGradEXT
			return texture2DGradEXT( envMap, uv, vec2( 0.0 ), vec2( 0.0 ) ).rgb;
		#else
			return texture2D( envMap, uv ).rgb;
		#endif
	}
	#define cubeUV_r0 1.0
	#define cubeUV_m0 - 2.0
	#define cubeUV_r1 0.8
	#define cubeUV_m1 - 1.0
	#define cubeUV_r4 0.4
	#define cubeUV_m4 2.0
	#define cubeUV_r5 0.305
	#define cubeUV_m5 3.0
	#define cubeUV_r6 0.21
	#define cubeUV_m6 4.0
	float roughnessToMip( float roughness ) {
		float mip = 0.0;
		if ( roughness >= cubeUV_r1 ) {
			mip = ( cubeUV_r0 - roughness ) * ( cubeUV_m1 - cubeUV_m0 ) / ( cubeUV_r0 - cubeUV_r1 ) + cubeUV_m0;
		} else if ( roughness >= cubeUV_r4 ) {
			mip = ( cubeUV_r1 - roughness ) * ( cubeUV_m4 - cubeUV_m1 ) / ( cubeUV_r1 - cubeUV_r4 ) + cubeUV_m1;
		} else if ( roughness >= cubeUV_r5 ) {
			mip = ( cubeUV_r4 - roughness ) * ( cubeUV_m5 - cubeUV_m4 ) / ( cubeUV_r4 - cubeUV_r5 ) + cubeUV_m4;
		} else if ( roughness >= cubeUV_r6 ) {
			mip = ( cubeUV_r5 - roughness ) * ( cubeUV_m6 - cubeUV_m5 ) / ( cubeUV_r5 - cubeUV_r6 ) + cubeUV_m5;
		} else {
			mip = - 2.0 * log2( 1.16 * roughness );		}
		return mip;
	}
	vec4 textureCubeUV( sampler2D envMap, vec3 sampleDir, float roughness ) {
		float mip = clamp( roughnessToMip( roughness ), cubeUV_m0, CUBEUV_MAX_MIP );
		float mipF = fract( mip );
		float mipInt = floor( mip );
		vec3 color0 = bilinearCubeUV( envMap, sampleDir, mipInt );
		if ( mipF == 0.0 ) {
			return vec4( color0, 1.0 );
		} else {
			vec3 color1 = bilinearCubeUV( envMap, sampleDir, mipInt + 1.0 );
			return vec4( mix( color0, color1, mipF ), 1.0 );
		}
	}
#endif`,gv=`vec3 transformedNormal = objectNormal;
#ifdef USE_TANGENT
	vec3 transformedTangent = objectTangent;
#endif
#ifdef USE_BATCHING
	mat3 bm = mat3( batchingMatrix );
	transformedNormal /= vec3( dot( bm[ 0 ], bm[ 0 ] ), dot( bm[ 1 ], bm[ 1 ] ), dot( bm[ 2 ], bm[ 2 ] ) );
	transformedNormal = bm * transformedNormal;
	#ifdef USE_TANGENT
		transformedTangent = bm * transformedTangent;
	#endif
#endif
#ifdef USE_INSTANCING
	mat3 im = mat3( instanceMatrix );
	transformedNormal /= vec3( dot( im[ 0 ], im[ 0 ] ), dot( im[ 1 ], im[ 1 ] ), dot( im[ 2 ], im[ 2 ] ) );
	transformedNormal = im * transformedNormal;
	#ifdef USE_TANGENT
		transformedTangent = im * transformedTangent;
	#endif
#endif
transformedNormal = normalMatrix * transformedNormal;
#ifdef FLIP_SIDED
	transformedNormal = - transformedNormal;
#endif
#ifdef USE_TANGENT
	transformedTangent = ( modelViewMatrix * vec4( transformedTangent, 0.0 ) ).xyz;
	#ifdef FLIP_SIDED
		transformedTangent = - transformedTangent;
	#endif
#endif`,_v=`#ifdef USE_DISPLACEMENTMAP
	uniform sampler2D displacementMap;
	uniform float displacementScale;
	uniform float displacementBias;
#endif`,yv=`#ifdef USE_DISPLACEMENTMAP
	transformed += normalize( objectNormal ) * ( texture2D( displacementMap, vDisplacementMapUv ).x * displacementScale + displacementBias );
#endif`,vv=`#ifdef USE_EMISSIVEMAP
	vec4 emissiveColor = texture2D( emissiveMap, vEmissiveMapUv );
	#ifdef DECODE_VIDEO_TEXTURE_EMISSIVE
		emissiveColor = sRGBTransferEOTF( emissiveColor );
	#endif
	totalEmissiveRadiance *= emissiveColor.rgb;
#endif`,xv=`#ifdef USE_EMISSIVEMAP
	uniform sampler2D emissiveMap;
#endif`,Sv="gl_FragColor = linearToOutputTexel( gl_FragColor );",bv=`vec4 LinearTransferOETF( in vec4 value ) {
	return value;
}
vec4 sRGBTransferEOTF( in vec4 value ) {
	return vec4( mix( pow( value.rgb * 0.9478672986 + vec3( 0.0521327014 ), vec3( 2.4 ) ), value.rgb * 0.0773993808, vec3( lessThanEqual( value.rgb, vec3( 0.04045 ) ) ) ), value.a );
}
vec4 sRGBTransferOETF( in vec4 value ) {
	return vec4( mix( pow( value.rgb, vec3( 0.41666 ) ) * 1.055 - vec3( 0.055 ), value.rgb * 12.92, vec3( lessThanEqual( value.rgb, vec3( 0.0031308 ) ) ) ), value.a );
}`,Mv=`#ifdef USE_ENVMAP
	#ifdef ENV_WORLDPOS
		vec3 cameraToFrag;
		if ( isOrthographic ) {
			cameraToFrag = normalize( vec3( - viewMatrix[ 0 ][ 2 ], - viewMatrix[ 1 ][ 2 ], - viewMatrix[ 2 ][ 2 ] ) );
		} else {
			cameraToFrag = normalize( vWorldPosition - cameraPosition );
		}
		vec3 worldNormal = inverseTransformDirection( normal, viewMatrix );
		#ifdef ENVMAP_MODE_REFLECTION
			vec3 reflectVec = reflect( cameraToFrag, worldNormal );
		#else
			vec3 reflectVec = refract( cameraToFrag, worldNormal, refractionRatio );
		#endif
	#else
		vec3 reflectVec = vReflect;
	#endif
	#ifdef ENVMAP_TYPE_CUBE
		vec4 envColor = textureCube( envMap, envMapRotation * vec3( flipEnvMap * reflectVec.x, reflectVec.yz ) );
	#else
		vec4 envColor = vec4( 0.0 );
	#endif
	#ifdef ENVMAP_BLENDING_MULTIPLY
		outgoingLight = mix( outgoingLight, outgoingLight * envColor.xyz, specularStrength * reflectivity );
	#elif defined( ENVMAP_BLENDING_MIX )
		outgoingLight = mix( outgoingLight, envColor.xyz, specularStrength * reflectivity );
	#elif defined( ENVMAP_BLENDING_ADD )
		outgoingLight += envColor.xyz * specularStrength * reflectivity;
	#endif
#endif`,Ev=`#ifdef USE_ENVMAP
	uniform float envMapIntensity;
	uniform float flipEnvMap;
	uniform mat3 envMapRotation;
	#ifdef ENVMAP_TYPE_CUBE
		uniform samplerCube envMap;
	#else
		uniform sampler2D envMap;
	#endif
	
#endif`,wv=`#ifdef USE_ENVMAP
	uniform float reflectivity;
	#if defined( USE_BUMPMAP ) || defined( USE_NORMALMAP ) || defined( PHONG ) || defined( LAMBERT )
		#define ENV_WORLDPOS
	#endif
	#ifdef ENV_WORLDPOS
		varying vec3 vWorldPosition;
		uniform float refractionRatio;
	#else
		varying vec3 vReflect;
	#endif
#endif`,Tv=`#ifdef USE_ENVMAP
	#if defined( USE_BUMPMAP ) || defined( USE_NORMALMAP ) || defined( PHONG ) || defined( LAMBERT )
		#define ENV_WORLDPOS
	#endif
	#ifdef ENV_WORLDPOS
		
		varying vec3 vWorldPosition;
	#else
		varying vec3 vReflect;
		uniform float refractionRatio;
	#endif
#endif`,Av=`#ifdef USE_ENVMAP
	#ifdef ENV_WORLDPOS
		vWorldPosition = worldPosition.xyz;
	#else
		vec3 cameraToVertex;
		if ( isOrthographic ) {
			cameraToVertex = normalize( vec3( - viewMatrix[ 0 ][ 2 ], - viewMatrix[ 1 ][ 2 ], - viewMatrix[ 2 ][ 2 ] ) );
		} else {
			cameraToVertex = normalize( worldPosition.xyz - cameraPosition );
		}
		vec3 worldNormal = inverseTransformDirection( transformedNormal, viewMatrix );
		#ifdef ENVMAP_MODE_REFLECTION
			vReflect = reflect( cameraToVertex, worldNormal );
		#else
			vReflect = refract( cameraToVertex, worldNormal, refractionRatio );
		#endif
	#endif
#endif`,Rv=`#ifdef USE_FOG
	vFogDepth = - mvPosition.z;
#endif`,Cv=`#ifdef USE_FOG
	varying float vFogDepth;
#endif`,Pv=`#ifdef USE_FOG
	#ifdef FOG_EXP2
		float fogFactor = 1.0 - exp( - fogDensity * fogDensity * vFogDepth * vFogDepth );
	#else
		float fogFactor = smoothstep( fogNear, fogFar, vFogDepth );
	#endif
	gl_FragColor.rgb = mix( gl_FragColor.rgb, fogColor, fogFactor );
#endif`,Iv=`#ifdef USE_FOG
	uniform vec3 fogColor;
	varying float vFogDepth;
	#ifdef FOG_EXP2
		uniform float fogDensity;
	#else
		uniform float fogNear;
		uniform float fogFar;
	#endif
#endif`,Dv=`#ifdef USE_GRADIENTMAP
	uniform sampler2D gradientMap;
#endif
vec3 getGradientIrradiance( vec3 normal, vec3 lightDirection ) {
	float dotNL = dot( normal, lightDirection );
	vec2 coord = vec2( dotNL * 0.5 + 0.5, 0.0 );
	#ifdef USE_GRADIENTMAP
		return vec3( texture2D( gradientMap, coord ).r );
	#else
		vec2 fw = fwidth( coord ) * 0.5;
		return mix( vec3( 0.7 ), vec3( 1.0 ), smoothstep( 0.7 - fw.x, 0.7 + fw.x, coord.x ) );
	#endif
}`,Nv=`#ifdef USE_LIGHTMAP
	uniform sampler2D lightMap;
	uniform float lightMapIntensity;
#endif`,Lv=`LambertMaterial material;
material.diffuseColor = diffuseColor.rgb;
material.specularStrength = specularStrength;`,Uv=`varying vec3 vViewPosition;
struct LambertMaterial {
	vec3 diffuseColor;
	float specularStrength;
};
void RE_Direct_Lambert( const in IncidentLight directLight, const in vec3 geometryPosition, const in vec3 geometryNormal, const in vec3 geometryViewDir, const in vec3 geometryClearcoatNormal, const in LambertMaterial material, inout ReflectedLight reflectedLight ) {
	float dotNL = saturate( dot( geometryNormal, directLight.direction ) );
	vec3 irradiance = dotNL * directLight.color;
	reflectedLight.directDiffuse += irradiance * BRDF_Lambert( material.diffuseColor );
}
void RE_IndirectDiffuse_Lambert( const in vec3 irradiance, const in vec3 geometryPosition, const in vec3 geometryNormal, const in vec3 geometryViewDir, const in vec3 geometryClearcoatNormal, const in LambertMaterial material, inout ReflectedLight reflectedLight ) {
	reflectedLight.indirectDiffuse += irradiance * BRDF_Lambert( material.diffuseColor );
}
#define RE_Direct				RE_Direct_Lambert
#define RE_IndirectDiffuse		RE_IndirectDiffuse_Lambert`,Ov=`uniform bool receiveShadow;
uniform vec3 ambientLightColor;
#if defined( USE_LIGHT_PROBES )
	uniform vec3 lightProbe[ 9 ];
#endif
vec3 shGetIrradianceAt( in vec3 normal, in vec3 shCoefficients[ 9 ] ) {
	float x = normal.x, y = normal.y, z = normal.z;
	vec3 result = shCoefficients[ 0 ] * 0.886227;
	result += shCoefficients[ 1 ] * 2.0 * 0.511664 * y;
	result += shCoefficients[ 2 ] * 2.0 * 0.511664 * z;
	result += shCoefficients[ 3 ] * 2.0 * 0.511664 * x;
	result += shCoefficients[ 4 ] * 2.0 * 0.429043 * x * y;
	result += shCoefficients[ 5 ] * 2.0 * 0.429043 * y * z;
	result += shCoefficients[ 6 ] * ( 0.743125 * z * z - 0.247708 );
	result += shCoefficients[ 7 ] * 2.0 * 0.429043 * x * z;
	result += shCoefficients[ 8 ] * 0.429043 * ( x * x - y * y );
	return result;
}
vec3 getLightProbeIrradiance( const in vec3 lightProbe[ 9 ], const in vec3 normal ) {
	vec3 worldNormal = inverseTransformDirection( normal, viewMatrix );
	vec3 irradiance = shGetIrradianceAt( worldNormal, lightProbe );
	return irradiance;
}
vec3 getAmbientLightIrradiance( const in vec3 ambientLightColor ) {
	vec3 irradiance = ambientLightColor;
	return irradiance;
}
float getDistanceAttenuation( const in float lightDistance, const in float cutoffDistance, const in float decayExponent ) {
	float distanceFalloff = 1.0 / max( pow( lightDistance, decayExponent ), 0.01 );
	if ( cutoffDistance > 0.0 ) {
		distanceFalloff *= pow2( saturate( 1.0 - pow4( lightDistance / cutoffDistance ) ) );
	}
	return distanceFalloff;
}
float getSpotAttenuation( const in float coneCosine, const in float penumbraCosine, const in float angleCosine ) {
	return smoothstep( coneCosine, penumbraCosine, angleCosine );
}
#if NUM_DIR_LIGHTS > 0
	struct DirectionalLight {
		vec3 direction;
		vec3 color;
	};
	uniform DirectionalLight directionalLights[ NUM_DIR_LIGHTS ];
	void getDirectionalLightInfo( const in DirectionalLight directionalLight, out IncidentLight light ) {
		light.color = directionalLight.color;
		light.direction = directionalLight.direction;
		light.visible = true;
	}
#endif
#if NUM_POINT_LIGHTS > 0
	struct PointLight {
		vec3 position;
		vec3 color;
		float distance;
		float decay;
	};
	uniform PointLight pointLights[ NUM_POINT_LIGHTS ];
	void getPointLightInfo( const in PointLight pointLight, const in vec3 geometryPosition, out IncidentLight light ) {
		vec3 lVector = pointLight.position - geometryPosition;
		light.direction = normalize( lVector );
		float lightDistance = length( lVector );
		light.color = pointLight.color;
		light.color *= getDistanceAttenuation( lightDistance, pointLight.distance, pointLight.decay );
		light.visible = ( light.color != vec3( 0.0 ) );
	}
#endif
#if NUM_SPOT_LIGHTS > 0
	struct SpotLight {
		vec3 position;
		vec3 direction;
		vec3 color;
		float distance;
		float decay;
		float coneCos;
		float penumbraCos;
	};
	uniform SpotLight spotLights[ NUM_SPOT_LIGHTS ];
	void getSpotLightInfo( const in SpotLight spotLight, const in vec3 geometryPosition, out IncidentLight light ) {
		vec3 lVector = spotLight.position - geometryPosition;
		light.direction = normalize( lVector );
		float angleCos = dot( light.direction, spotLight.direction );
		float spotAttenuation = getSpotAttenuation( spotLight.coneCos, spotLight.penumbraCos, angleCos );
		if ( spotAttenuation > 0.0 ) {
			float lightDistance = length( lVector );
			light.color = spotLight.color * spotAttenuation;
			light.color *= getDistanceAttenuation( lightDistance, spotLight.distance, spotLight.decay );
			light.visible = ( light.color != vec3( 0.0 ) );
		} else {
			light.color = vec3( 0.0 );
			light.visible = false;
		}
	}
#endif
#if NUM_RECT_AREA_LIGHTS > 0
	struct RectAreaLight {
		vec3 color;
		vec3 position;
		vec3 halfWidth;
		vec3 halfHeight;
	};
	uniform sampler2D ltc_1;	uniform sampler2D ltc_2;
	uniform RectAreaLight rectAreaLights[ NUM_RECT_AREA_LIGHTS ];
#endif
#if NUM_HEMI_LIGHTS > 0
	struct HemisphereLight {
		vec3 direction;
		vec3 skyColor;
		vec3 groundColor;
	};
	uniform HemisphereLight hemisphereLights[ NUM_HEMI_LIGHTS ];
	vec3 getHemisphereLightIrradiance( const in HemisphereLight hemiLight, const in vec3 normal ) {
		float dotNL = dot( normal, hemiLight.direction );
		float hemiDiffuseWeight = 0.5 * dotNL + 0.5;
		vec3 irradiance = mix( hemiLight.groundColor, hemiLight.skyColor, hemiDiffuseWeight );
		return irradiance;
	}
#endif`,Fv=`#ifdef USE_ENVMAP
	vec3 getIBLIrradiance( const in vec3 normal ) {
		#ifdef ENVMAP_TYPE_CUBE_UV
			vec3 worldNormal = inverseTransformDirection( normal, viewMatrix );
			vec4 envMapColor = textureCubeUV( envMap, envMapRotation * worldNormal, 1.0 );
			return PI * envMapColor.rgb * envMapIntensity;
		#else
			return vec3( 0.0 );
		#endif
	}
	vec3 getIBLRadiance( const in vec3 viewDir, const in vec3 normal, const in float roughness ) {
		#ifdef ENVMAP_TYPE_CUBE_UV
			vec3 reflectVec = reflect( - viewDir, normal );
			reflectVec = normalize( mix( reflectVec, normal, roughness * roughness) );
			reflectVec = inverseTransformDirection( reflectVec, viewMatrix );
			vec4 envMapColor = textureCubeUV( envMap, envMapRotation * reflectVec, roughness );
			return envMapColor.rgb * envMapIntensity;
		#else
			return vec3( 0.0 );
		#endif
	}
	#ifdef USE_ANISOTROPY
		vec3 getIBLAnisotropyRadiance( const in vec3 viewDir, const in vec3 normal, const in float roughness, const in vec3 bitangent, const in float anisotropy ) {
			#ifdef ENVMAP_TYPE_CUBE_UV
				vec3 bentNormal = cross( bitangent, viewDir );
				bentNormal = normalize( cross( bentNormal, bitangent ) );
				bentNormal = normalize( mix( bentNormal, normal, pow2( pow2( 1.0 - anisotropy * ( 1.0 - roughness ) ) ) ) );
				return getIBLRadiance( viewDir, bentNormal, roughness );
			#else
				return vec3( 0.0 );
			#endif
		}
	#endif
#endif`,Bv=`ToonMaterial material;
material.diffuseColor = diffuseColor.rgb;`,kv=`varying vec3 vViewPosition;
struct ToonMaterial {
	vec3 diffuseColor;
};
void RE_Direct_Toon( const in IncidentLight directLight, const in vec3 geometryPosition, const in vec3 geometryNormal, const in vec3 geometryViewDir, const in vec3 geometryClearcoatNormal, const in ToonMaterial material, inout ReflectedLight reflectedLight ) {
	vec3 irradiance = getGradientIrradiance( geometryNormal, directLight.direction ) * directLight.color;
	reflectedLight.directDiffuse += irradiance * BRDF_Lambert( material.diffuseColor );
}
void RE_IndirectDiffuse_Toon( const in vec3 irradiance, const in vec3 geometryPosition, const in vec3 geometryNormal, const in vec3 geometryViewDir, const in vec3 geometryClearcoatNormal, const in ToonMaterial material, inout ReflectedLight reflectedLight ) {
	reflectedLight.indirectDiffuse += irradiance * BRDF_Lambert( material.diffuseColor );
}
#define RE_Direct				RE_Direct_Toon
#define RE_IndirectDiffuse		RE_IndirectDiffuse_Toon`,zv=`BlinnPhongMaterial material;
material.diffuseColor = diffuseColor.rgb;
material.specularColor = specular;
material.specularShininess = shininess;
material.specularStrength = specularStrength;`,Vv=`varying vec3 vViewPosition;
struct BlinnPhongMaterial {
	vec3 diffuseColor;
	vec3 specularColor;
	float specularShininess;
	float specularStrength;
};
void RE_Direct_BlinnPhong( const in IncidentLight directLight, const in vec3 geometryPosition, const in vec3 geometryNormal, const in vec3 geometryViewDir, const in vec3 geometryClearcoatNormal, const in BlinnPhongMaterial material, inout ReflectedLight reflectedLight ) {
	float dotNL = saturate( dot( geometryNormal, directLight.direction ) );
	vec3 irradiance = dotNL * directLight.color;
	reflectedLight.directDiffuse += irradiance * BRDF_Lambert( material.diffuseColor );
	reflectedLight.directSpecular += irradiance * BRDF_BlinnPhong( directLight.direction, geometryViewDir, geometryNormal, material.specularColor, material.specularShininess ) * material.specularStrength;
}
void RE_IndirectDiffuse_BlinnPhong( const in vec3 irradiance, const in vec3 geometryPosition, const in vec3 geometryNormal, const in vec3 geometryViewDir, const in vec3 geometryClearcoatNormal, const in BlinnPhongMaterial material, inout ReflectedLight reflectedLight ) {
	reflectedLight.indirectDiffuse += irradiance * BRDF_Lambert( material.diffuseColor );
}
#define RE_Direct				RE_Direct_BlinnPhong
#define RE_IndirectDiffuse		RE_IndirectDiffuse_BlinnPhong`,$v=`PhysicalMaterial material;
material.diffuseColor = diffuseColor.rgb * ( 1.0 - metalnessFactor );
vec3 dxy = max( abs( dFdx( nonPerturbedNormal ) ), abs( dFdy( nonPerturbedNormal ) ) );
float geometryRoughness = max( max( dxy.x, dxy.y ), dxy.z );
material.roughness = max( roughnessFactor, 0.0525 );material.roughness += geometryRoughness;
material.roughness = min( material.roughness, 1.0 );
#ifdef IOR
	material.ior = ior;
	#ifdef USE_SPECULAR
		float specularIntensityFactor = specularIntensity;
		vec3 specularColorFactor = specularColor;
		#ifdef USE_SPECULAR_COLORMAP
			specularColorFactor *= texture2D( specularColorMap, vSpecularColorMapUv ).rgb;
		#endif
		#ifdef USE_SPECULAR_INTENSITYMAP
			specularIntensityFactor *= texture2D( specularIntensityMap, vSpecularIntensityMapUv ).a;
		#endif
		material.specularF90 = mix( specularIntensityFactor, 1.0, metalnessFactor );
	#else
		float specularIntensityFactor = 1.0;
		vec3 specularColorFactor = vec3( 1.0 );
		material.specularF90 = 1.0;
	#endif
	material.specularColor = mix( min( pow2( ( material.ior - 1.0 ) / ( material.ior + 1.0 ) ) * specularColorFactor, vec3( 1.0 ) ) * specularIntensityFactor, diffuseColor.rgb, metalnessFactor );
#else
	material.specularColor = mix( vec3( 0.04 ), diffuseColor.rgb, metalnessFactor );
	material.specularF90 = 1.0;
#endif
#ifdef USE_CLEARCOAT
	material.clearcoat = clearcoat;
	material.clearcoatRoughness = clearcoatRoughness;
	material.clearcoatF0 = vec3( 0.04 );
	material.clearcoatF90 = 1.0;
	#ifdef USE_CLEARCOATMAP
		material.clearcoat *= texture2D( clearcoatMap, vClearcoatMapUv ).x;
	#endif
	#ifdef USE_CLEARCOAT_ROUGHNESSMAP
		material.clearcoatRoughness *= texture2D( clearcoatRoughnessMap, vClearcoatRoughnessMapUv ).y;
	#endif
	material.clearcoat = saturate( material.clearcoat );	material.clearcoatRoughness = max( material.clearcoatRoughness, 0.0525 );
	material.clearcoatRoughness += geometryRoughness;
	material.clearcoatRoughness = min( material.clearcoatRoughness, 1.0 );
#endif
#ifdef USE_DISPERSION
	material.dispersion = dispersion;
#endif
#ifdef USE_IRIDESCENCE
	material.iridescence = iridescence;
	material.iridescenceIOR = iridescenceIOR;
	#ifdef USE_IRIDESCENCEMAP
		material.iridescence *= texture2D( iridescenceMap, vIridescenceMapUv ).r;
	#endif
	#ifdef USE_IRIDESCENCE_THICKNESSMAP
		material.iridescenceThickness = (iridescenceThicknessMaximum - iridescenceThicknessMinimum) * texture2D( iridescenceThicknessMap, vIridescenceThicknessMapUv ).g + iridescenceThicknessMinimum;
	#else
		material.iridescenceThickness = iridescenceThicknessMaximum;
	#endif
#endif
#ifdef USE_SHEEN
	material.sheenColor = sheenColor;
	#ifdef USE_SHEEN_COLORMAP
		material.sheenColor *= texture2D( sheenColorMap, vSheenColorMapUv ).rgb;
	#endif
	material.sheenRoughness = clamp( sheenRoughness, 0.07, 1.0 );
	#ifdef USE_SHEEN_ROUGHNESSMAP
		material.sheenRoughness *= texture2D( sheenRoughnessMap, vSheenRoughnessMapUv ).a;
	#endif
#endif
#ifdef USE_ANISOTROPY
	#ifdef USE_ANISOTROPYMAP
		mat2 anisotropyMat = mat2( anisotropyVector.x, anisotropyVector.y, - anisotropyVector.y, anisotropyVector.x );
		vec3 anisotropyPolar = texture2D( anisotropyMap, vAnisotropyMapUv ).rgb;
		vec2 anisotropyV = anisotropyMat * normalize( 2.0 * anisotropyPolar.rg - vec2( 1.0 ) ) * anisotropyPolar.b;
	#else
		vec2 anisotropyV = anisotropyVector;
	#endif
	material.anisotropy = length( anisotropyV );
	if( material.anisotropy == 0.0 ) {
		anisotropyV = vec2( 1.0, 0.0 );
	} else {
		anisotropyV /= material.anisotropy;
		material.anisotropy = saturate( material.anisotropy );
	}
	material.alphaT = mix( pow2( material.roughness ), 1.0, pow2( material.anisotropy ) );
	material.anisotropyT = tbn[ 0 ] * anisotropyV.x + tbn[ 1 ] * anisotropyV.y;
	material.anisotropyB = tbn[ 1 ] * anisotropyV.x - tbn[ 0 ] * anisotropyV.y;
#endif`,Hv=`struct PhysicalMaterial {
	vec3 diffuseColor;
	float roughness;
	vec3 specularColor;
	float specularF90;
	float dispersion;
	#ifdef USE_CLEARCOAT
		float clearcoat;
		float clearcoatRoughness;
		vec3 clearcoatF0;
		float clearcoatF90;
	#endif
	#ifdef USE_IRIDESCENCE
		float iridescence;
		float iridescenceIOR;
		float iridescenceThickness;
		vec3 iridescenceFresnel;
		vec3 iridescenceF0;
	#endif
	#ifdef USE_SHEEN
		vec3 sheenColor;
		float sheenRoughness;
	#endif
	#ifdef IOR
		float ior;
	#endif
	#ifdef USE_TRANSMISSION
		float transmission;
		float transmissionAlpha;
		float thickness;
		float attenuationDistance;
		vec3 attenuationColor;
	#endif
	#ifdef USE_ANISOTROPY
		float anisotropy;
		float alphaT;
		vec3 anisotropyT;
		vec3 anisotropyB;
	#endif
};
vec3 clearcoatSpecularDirect = vec3( 0.0 );
vec3 clearcoatSpecularIndirect = vec3( 0.0 );
vec3 sheenSpecularDirect = vec3( 0.0 );
vec3 sheenSpecularIndirect = vec3(0.0 );
vec3 Schlick_to_F0( const in vec3 f, const in float f90, const in float dotVH ) {
    float x = clamp( 1.0 - dotVH, 0.0, 1.0 );
    float x2 = x * x;
    float x5 = clamp( x * x2 * x2, 0.0, 0.9999 );
    return ( f - vec3( f90 ) * x5 ) / ( 1.0 - x5 );
}
float V_GGX_SmithCorrelated( const in float alpha, const in float dotNL, const in float dotNV ) {
	float a2 = pow2( alpha );
	float gv = dotNL * sqrt( a2 + ( 1.0 - a2 ) * pow2( dotNV ) );
	float gl = dotNV * sqrt( a2 + ( 1.0 - a2 ) * pow2( dotNL ) );
	return 0.5 / max( gv + gl, EPSILON );
}
float D_GGX( const in float alpha, const in float dotNH ) {
	float a2 = pow2( alpha );
	float denom = pow2( dotNH ) * ( a2 - 1.0 ) + 1.0;
	return RECIPROCAL_PI * a2 / pow2( denom );
}
#ifdef USE_ANISOTROPY
	float V_GGX_SmithCorrelated_Anisotropic( const in float alphaT, const in float alphaB, const in float dotTV, const in float dotBV, const in float dotTL, const in float dotBL, const in float dotNV, const in float dotNL ) {
		float gv = dotNL * length( vec3( alphaT * dotTV, alphaB * dotBV, dotNV ) );
		float gl = dotNV * length( vec3( alphaT * dotTL, alphaB * dotBL, dotNL ) );
		float v = 0.5 / ( gv + gl );
		return saturate(v);
	}
	float D_GGX_Anisotropic( const in float alphaT, const in float alphaB, const in float dotNH, const in float dotTH, const in float dotBH ) {
		float a2 = alphaT * alphaB;
		highp vec3 v = vec3( alphaB * dotTH, alphaT * dotBH, a2 * dotNH );
		highp float v2 = dot( v, v );
		float w2 = a2 / v2;
		return RECIPROCAL_PI * a2 * pow2 ( w2 );
	}
#endif
#ifdef USE_CLEARCOAT
	vec3 BRDF_GGX_Clearcoat( const in vec3 lightDir, const in vec3 viewDir, const in vec3 normal, const in PhysicalMaterial material) {
		vec3 f0 = material.clearcoatF0;
		float f90 = material.clearcoatF90;
		float roughness = material.clearcoatRoughness;
		float alpha = pow2( roughness );
		vec3 halfDir = normalize( lightDir + viewDir );
		float dotNL = saturate( dot( normal, lightDir ) );
		float dotNV = saturate( dot( normal, viewDir ) );
		float dotNH = saturate( dot( normal, halfDir ) );
		float dotVH = saturate( dot( viewDir, halfDir ) );
		vec3 F = F_Schlick( f0, f90, dotVH );
		float V = V_GGX_SmithCorrelated( alpha, dotNL, dotNV );
		float D = D_GGX( alpha, dotNH );
		return F * ( V * D );
	}
#endif
vec3 BRDF_GGX( const in vec3 lightDir, const in vec3 viewDir, const in vec3 normal, const in PhysicalMaterial material ) {
	vec3 f0 = material.specularColor;
	float f90 = material.specularF90;
	float roughness = material.roughness;
	float alpha = pow2( roughness );
	vec3 halfDir = normalize( lightDir + viewDir );
	float dotNL = saturate( dot( normal, lightDir ) );
	float dotNV = saturate( dot( normal, viewDir ) );
	float dotNH = saturate( dot( normal, halfDir ) );
	float dotVH = saturate( dot( viewDir, halfDir ) );
	vec3 F = F_Schlick( f0, f90, dotVH );
	#ifdef USE_IRIDESCENCE
		F = mix( F, material.iridescenceFresnel, material.iridescence );
	#endif
	#ifdef USE_ANISOTROPY
		float dotTL = dot( material.anisotropyT, lightDir );
		float dotTV = dot( material.anisotropyT, viewDir );
		float dotTH = dot( material.anisotropyT, halfDir );
		float dotBL = dot( material.anisotropyB, lightDir );
		float dotBV = dot( material.anisotropyB, viewDir );
		float dotBH = dot( material.anisotropyB, halfDir );
		float V = V_GGX_SmithCorrelated_Anisotropic( material.alphaT, alpha, dotTV, dotBV, dotTL, dotBL, dotNV, dotNL );
		float D = D_GGX_Anisotropic( material.alphaT, alpha, dotNH, dotTH, dotBH );
	#else
		float V = V_GGX_SmithCorrelated( alpha, dotNL, dotNV );
		float D = D_GGX( alpha, dotNH );
	#endif
	return F * ( V * D );
}
vec2 LTC_Uv( const in vec3 N, const in vec3 V, const in float roughness ) {
	const float LUT_SIZE = 64.0;
	const float LUT_SCALE = ( LUT_SIZE - 1.0 ) / LUT_SIZE;
	const float LUT_BIAS = 0.5 / LUT_SIZE;
	float dotNV = saturate( dot( N, V ) );
	vec2 uv = vec2( roughness, sqrt( 1.0 - dotNV ) );
	uv = uv * LUT_SCALE + LUT_BIAS;
	return uv;
}
float LTC_ClippedSphereFormFactor( const in vec3 f ) {
	float l = length( f );
	return max( ( l * l + f.z ) / ( l + 1.0 ), 0.0 );
}
vec3 LTC_EdgeVectorFormFactor( const in vec3 v1, const in vec3 v2 ) {
	float x = dot( v1, v2 );
	float y = abs( x );
	float a = 0.8543985 + ( 0.4965155 + 0.0145206 * y ) * y;
	float b = 3.4175940 + ( 4.1616724 + y ) * y;
	float v = a / b;
	float theta_sintheta = ( x > 0.0 ) ? v : 0.5 * inversesqrt( max( 1.0 - x * x, 1e-7 ) ) - v;
	return cross( v1, v2 ) * theta_sintheta;
}
vec3 LTC_Evaluate( const in vec3 N, const in vec3 V, const in vec3 P, const in mat3 mInv, const in vec3 rectCoords[ 4 ] ) {
	vec3 v1 = rectCoords[ 1 ] - rectCoords[ 0 ];
	vec3 v2 = rectCoords[ 3 ] - rectCoords[ 0 ];
	vec3 lightNormal = cross( v1, v2 );
	if( dot( lightNormal, P - rectCoords[ 0 ] ) < 0.0 ) return vec3( 0.0 );
	vec3 T1, T2;
	T1 = normalize( V - N * dot( V, N ) );
	T2 = - cross( N, T1 );
	mat3 mat = mInv * transposeMat3( mat3( T1, T2, N ) );
	vec3 coords[ 4 ];
	coords[ 0 ] = mat * ( rectCoords[ 0 ] - P );
	coords[ 1 ] = mat * ( rectCoords[ 1 ] - P );
	coords[ 2 ] = mat * ( rectCoords[ 2 ] - P );
	coords[ 3 ] = mat * ( rectCoords[ 3 ] - P );
	coords[ 0 ] = normalize( coords[ 0 ] );
	coords[ 1 ] = normalize( coords[ 1 ] );
	coords[ 2 ] = normalize( coords[ 2 ] );
	coords[ 3 ] = normalize( coords[ 3 ] );
	vec3 vectorFormFactor = vec3( 0.0 );
	vectorFormFactor += LTC_EdgeVectorFormFactor( coords[ 0 ], coords[ 1 ] );
	vectorFormFactor += LTC_EdgeVectorFormFactor( coords[ 1 ], coords[ 2 ] );
	vectorFormFactor += LTC_EdgeVectorFormFactor( coords[ 2 ], coords[ 3 ] );
	vectorFormFactor += LTC_EdgeVectorFormFactor( coords[ 3 ], coords[ 0 ] );
	float result = LTC_ClippedSphereFormFactor( vectorFormFactor );
	return vec3( result );
}
#if defined( USE_SHEEN )
float D_Charlie( float roughness, float dotNH ) {
	float alpha = pow2( roughness );
	float invAlpha = 1.0 / alpha;
	float cos2h = dotNH * dotNH;
	float sin2h = max( 1.0 - cos2h, 0.0078125 );
	return ( 2.0 + invAlpha ) * pow( sin2h, invAlpha * 0.5 ) / ( 2.0 * PI );
}
float V_Neubelt( float dotNV, float dotNL ) {
	return saturate( 1.0 / ( 4.0 * ( dotNL + dotNV - dotNL * dotNV ) ) );
}
vec3 BRDF_Sheen( const in vec3 lightDir, const in vec3 viewDir, const in vec3 normal, vec3 sheenColor, const in float sheenRoughness ) {
	vec3 halfDir = normalize( lightDir + viewDir );
	float dotNL = saturate( dot( normal, lightDir ) );
	float dotNV = saturate( dot( normal, viewDir ) );
	float dotNH = saturate( dot( normal, halfDir ) );
	float D = D_Charlie( sheenRoughness, dotNH );
	float V = V_Neubelt( dotNV, dotNL );
	return sheenColor * ( D * V );
}
#endif
float IBLSheenBRDF( const in vec3 normal, const in vec3 viewDir, const in float roughness ) {
	float dotNV = saturate( dot( normal, viewDir ) );
	float r2 = roughness * roughness;
	float a = roughness < 0.25 ? -339.2 * r2 + 161.4 * roughness - 25.9 : -8.48 * r2 + 14.3 * roughness - 9.95;
	float b = roughness < 0.25 ? 44.0 * r2 - 23.7 * roughness + 3.26 : 1.97 * r2 - 3.27 * roughness + 0.72;
	float DG = exp( a * dotNV + b ) + ( roughness < 0.25 ? 0.0 : 0.1 * ( roughness - 0.25 ) );
	return saturate( DG * RECIPROCAL_PI );
}
vec2 DFGApprox( const in vec3 normal, const in vec3 viewDir, const in float roughness ) {
	float dotNV = saturate( dot( normal, viewDir ) );
	const vec4 c0 = vec4( - 1, - 0.0275, - 0.572, 0.022 );
	const vec4 c1 = vec4( 1, 0.0425, 1.04, - 0.04 );
	vec4 r = roughness * c0 + c1;
	float a004 = min( r.x * r.x, exp2( - 9.28 * dotNV ) ) * r.x + r.y;
	vec2 fab = vec2( - 1.04, 1.04 ) * a004 + r.zw;
	return fab;
}
vec3 EnvironmentBRDF( const in vec3 normal, const in vec3 viewDir, const in vec3 specularColor, const in float specularF90, const in float roughness ) {
	vec2 fab = DFGApprox( normal, viewDir, roughness );
	return specularColor * fab.x + specularF90 * fab.y;
}
#ifdef USE_IRIDESCENCE
void computeMultiscatteringIridescence( const in vec3 normal, const in vec3 viewDir, const in vec3 specularColor, const in float specularF90, const in float iridescence, const in vec3 iridescenceF0, const in float roughness, inout vec3 singleScatter, inout vec3 multiScatter ) {
#else
void computeMultiscattering( const in vec3 normal, const in vec3 viewDir, const in vec3 specularColor, const in float specularF90, const in float roughness, inout vec3 singleScatter, inout vec3 multiScatter ) {
#endif
	vec2 fab = DFGApprox( normal, viewDir, roughness );
	#ifdef USE_IRIDESCENCE
		vec3 Fr = mix( specularColor, iridescenceF0, iridescence );
	#else
		vec3 Fr = specularColor;
	#endif
	vec3 FssEss = Fr * fab.x + specularF90 * fab.y;
	float Ess = fab.x + fab.y;
	float Ems = 1.0 - Ess;
	vec3 Favg = Fr + ( 1.0 - Fr ) * 0.047619;	vec3 Fms = FssEss * Favg / ( 1.0 - Ems * Favg );
	singleScatter += FssEss;
	multiScatter += Fms * Ems;
}
#if NUM_RECT_AREA_LIGHTS > 0
	void RE_Direct_RectArea_Physical( const in RectAreaLight rectAreaLight, const in vec3 geometryPosition, const in vec3 geometryNormal, const in vec3 geometryViewDir, const in vec3 geometryClearcoatNormal, const in PhysicalMaterial material, inout ReflectedLight reflectedLight ) {
		vec3 normal = geometryNormal;
		vec3 viewDir = geometryViewDir;
		vec3 position = geometryPosition;
		vec3 lightPos = rectAreaLight.position;
		vec3 halfWidth = rectAreaLight.halfWidth;
		vec3 halfHeight = rectAreaLight.halfHeight;
		vec3 lightColor = rectAreaLight.color;
		float roughness = material.roughness;
		vec3 rectCoords[ 4 ];
		rectCoords[ 0 ] = lightPos + halfWidth - halfHeight;		rectCoords[ 1 ] = lightPos - halfWidth - halfHeight;
		rectCoords[ 2 ] = lightPos - halfWidth + halfHeight;
		rectCoords[ 3 ] = lightPos + halfWidth + halfHeight;
		vec2 uv = LTC_Uv( normal, viewDir, roughness );
		vec4 t1 = texture2D( ltc_1, uv );
		vec4 t2 = texture2D( ltc_2, uv );
		mat3 mInv = mat3(
			vec3( t1.x, 0, t1.y ),
			vec3(    0, 1,    0 ),
			vec3( t1.z, 0, t1.w )
		);
		vec3 fresnel = ( material.specularColor * t2.x + ( vec3( 1.0 ) - material.specularColor ) * t2.y );
		reflectedLight.directSpecular += lightColor * fresnel * LTC_Evaluate( normal, viewDir, position, mInv, rectCoords );
		reflectedLight.directDiffuse += lightColor * material.diffuseColor * LTC_Evaluate( normal, viewDir, position, mat3( 1.0 ), rectCoords );
	}
#endif
void RE_Direct_Physical( const in IncidentLight directLight, const in vec3 geometryPosition, const in vec3 geometryNormal, const in vec3 geometryViewDir, const in vec3 geometryClearcoatNormal, const in PhysicalMaterial material, inout ReflectedLight reflectedLight ) {
	float dotNL = saturate( dot( geometryNormal, directLight.direction ) );
	vec3 irradiance = dotNL * directLight.color;
	#ifdef USE_CLEARCOAT
		float dotNLcc = saturate( dot( geometryClearcoatNormal, directLight.direction ) );
		vec3 ccIrradiance = dotNLcc * directLight.color;
		clearcoatSpecularDirect += ccIrradiance * BRDF_GGX_Clearcoat( directLight.direction, geometryViewDir, geometryClearcoatNormal, material );
	#endif
	#ifdef USE_SHEEN
		sheenSpecularDirect += irradiance * BRDF_Sheen( directLight.direction, geometryViewDir, geometryNormal, material.sheenColor, material.sheenRoughness );
	#endif
	reflectedLight.directSpecular += irradiance * BRDF_GGX( directLight.direction, geometryViewDir, geometryNormal, material );
	reflectedLight.directDiffuse += irradiance * BRDF_Lambert( material.diffuseColor );
}
void RE_IndirectDiffuse_Physical( const in vec3 irradiance, const in vec3 geometryPosition, const in vec3 geometryNormal, const in vec3 geometryViewDir, const in vec3 geometryClearcoatNormal, const in PhysicalMaterial material, inout ReflectedLight reflectedLight ) {
	reflectedLight.indirectDiffuse += irradiance * BRDF_Lambert( material.diffuseColor );
}
void RE_IndirectSpecular_Physical( const in vec3 radiance, const in vec3 irradiance, const in vec3 clearcoatRadiance, const in vec3 geometryPosition, const in vec3 geometryNormal, const in vec3 geometryViewDir, const in vec3 geometryClearcoatNormal, const in PhysicalMaterial material, inout ReflectedLight reflectedLight) {
	#ifdef USE_CLEARCOAT
		clearcoatSpecularIndirect += clearcoatRadiance * EnvironmentBRDF( geometryClearcoatNormal, geometryViewDir, material.clearcoatF0, material.clearcoatF90, material.clearcoatRoughness );
	#endif
	#ifdef USE_SHEEN
		sheenSpecularIndirect += irradiance * material.sheenColor * IBLSheenBRDF( geometryNormal, geometryViewDir, material.sheenRoughness );
	#endif
	vec3 singleScattering = vec3( 0.0 );
	vec3 multiScattering = vec3( 0.0 );
	vec3 cosineWeightedIrradiance = irradiance * RECIPROCAL_PI;
	#ifdef USE_IRIDESCENCE
		computeMultiscatteringIridescence( geometryNormal, geometryViewDir, material.specularColor, material.specularF90, material.iridescence, material.iridescenceFresnel, material.roughness, singleScattering, multiScattering );
	#else
		computeMultiscattering( geometryNormal, geometryViewDir, material.specularColor, material.specularF90, material.roughness, singleScattering, multiScattering );
	#endif
	vec3 totalScattering = singleScattering + multiScattering;
	vec3 diffuse = material.diffuseColor * ( 1.0 - max( max( totalScattering.r, totalScattering.g ), totalScattering.b ) );
	reflectedLight.indirectSpecular += radiance * singleScattering;
	reflectedLight.indirectSpecular += multiScattering * cosineWeightedIrradiance;
	reflectedLight.indirectDiffuse += diffuse * cosineWeightedIrradiance;
}
#define RE_Direct				RE_Direct_Physical
#define RE_Direct_RectArea		RE_Direct_RectArea_Physical
#define RE_IndirectDiffuse		RE_IndirectDiffuse_Physical
#define RE_IndirectSpecular		RE_IndirectSpecular_Physical
float computeSpecularOcclusion( const in float dotNV, const in float ambientOcclusion, const in float roughness ) {
	return saturate( pow( dotNV + ambientOcclusion, exp2( - 16.0 * roughness - 1.0 ) ) - 1.0 + ambientOcclusion );
}`,Gv=`
vec3 geometryPosition = - vViewPosition;
vec3 geometryNormal = normal;
vec3 geometryViewDir = ( isOrthographic ) ? vec3( 0, 0, 1 ) : normalize( vViewPosition );
vec3 geometryClearcoatNormal = vec3( 0.0 );
#ifdef USE_CLEARCOAT
	geometryClearcoatNormal = clearcoatNormal;
#endif
#ifdef USE_IRIDESCENCE
	float dotNVi = saturate( dot( normal, geometryViewDir ) );
	if ( material.iridescenceThickness == 0.0 ) {
		material.iridescence = 0.0;
	} else {
		material.iridescence = saturate( material.iridescence );
	}
	if ( material.iridescence > 0.0 ) {
		material.iridescenceFresnel = evalIridescence( 1.0, material.iridescenceIOR, dotNVi, material.iridescenceThickness, material.specularColor );
		material.iridescenceF0 = Schlick_to_F0( material.iridescenceFresnel, 1.0, dotNVi );
	}
#endif
IncidentLight directLight;
#if ( NUM_POINT_LIGHTS > 0 ) && defined( RE_Direct )
	PointLight pointLight;
	#if defined( USE_SHADOWMAP ) && NUM_POINT_LIGHT_SHADOWS > 0
	PointLightShadow pointLightShadow;
	#endif
	#pragma unroll_loop_start
	for ( int i = 0; i < NUM_POINT_LIGHTS; i ++ ) {
		pointLight = pointLights[ i ];
		getPointLightInfo( pointLight, geometryPosition, directLight );
		#if defined( USE_SHADOWMAP ) && ( UNROLLED_LOOP_INDEX < NUM_POINT_LIGHT_SHADOWS )
		pointLightShadow = pointLightShadows[ i ];
		directLight.color *= ( directLight.visible && receiveShadow ) ? getPointShadow( pointShadowMap[ i ], pointLightShadow.shadowMapSize, pointLightShadow.shadowIntensity, pointLightShadow.shadowBias, pointLightShadow.shadowRadius, vPointShadowCoord[ i ], pointLightShadow.shadowCameraNear, pointLightShadow.shadowCameraFar ) : 1.0;
		#endif
		RE_Direct( directLight, geometryPosition, geometryNormal, geometryViewDir, geometryClearcoatNormal, material, reflectedLight );
	}
	#pragma unroll_loop_end
#endif
#if ( NUM_SPOT_LIGHTS > 0 ) && defined( RE_Direct )
	SpotLight spotLight;
	vec4 spotColor;
	vec3 spotLightCoord;
	bool inSpotLightMap;
	#if defined( USE_SHADOWMAP ) && NUM_SPOT_LIGHT_SHADOWS > 0
	SpotLightShadow spotLightShadow;
	#endif
	#pragma unroll_loop_start
	for ( int i = 0; i < NUM_SPOT_LIGHTS; i ++ ) {
		spotLight = spotLights[ i ];
		getSpotLightInfo( spotLight, geometryPosition, directLight );
		#if ( UNROLLED_LOOP_INDEX < NUM_SPOT_LIGHT_SHADOWS_WITH_MAPS )
		#define SPOT_LIGHT_MAP_INDEX UNROLLED_LOOP_INDEX
		#elif ( UNROLLED_LOOP_INDEX < NUM_SPOT_LIGHT_SHADOWS )
		#define SPOT_LIGHT_MAP_INDEX NUM_SPOT_LIGHT_MAPS
		#else
		#define SPOT_LIGHT_MAP_INDEX ( UNROLLED_LOOP_INDEX - NUM_SPOT_LIGHT_SHADOWS + NUM_SPOT_LIGHT_SHADOWS_WITH_MAPS )
		#endif
		#if ( SPOT_LIGHT_MAP_INDEX < NUM_SPOT_LIGHT_MAPS )
			spotLightCoord = vSpotLightCoord[ i ].xyz / vSpotLightCoord[ i ].w;
			inSpotLightMap = all( lessThan( abs( spotLightCoord * 2. - 1. ), vec3( 1.0 ) ) );
			spotColor = texture2D( spotLightMap[ SPOT_LIGHT_MAP_INDEX ], spotLightCoord.xy );
			directLight.color = inSpotLightMap ? directLight.color * spotColor.rgb : directLight.color;
		#endif
		#undef SPOT_LIGHT_MAP_INDEX
		#if defined( USE_SHADOWMAP ) && ( UNROLLED_LOOP_INDEX < NUM_SPOT_LIGHT_SHADOWS )
		spotLightShadow = spotLightShadows[ i ];
		directLight.color *= ( directLight.visible && receiveShadow ) ? getShadow( spotShadowMap[ i ], spotLightShadow.shadowMapSize, spotLightShadow.shadowIntensity, spotLightShadow.shadowBias, spotLightShadow.shadowRadius, vSpotLightCoord[ i ] ) : 1.0;
		#endif
		RE_Direct( directLight, geometryPosition, geometryNormal, geometryViewDir, geometryClearcoatNormal, material, reflectedLight );
	}
	#pragma unroll_loop_end
#endif
#if ( NUM_DIR_LIGHTS > 0 ) && defined( RE_Direct )
	DirectionalLight directionalLight;
	#if defined( USE_SHADOWMAP ) && NUM_DIR_LIGHT_SHADOWS > 0
	DirectionalLightShadow directionalLightShadow;
	#endif
	#pragma unroll_loop_start
	for ( int i = 0; i < NUM_DIR_LIGHTS; i ++ ) {
		directionalLight = directionalLights[ i ];
		getDirectionalLightInfo( directionalLight, directLight );
		#if defined( USE_SHADOWMAP ) && ( UNROLLED_LOOP_INDEX < NUM_DIR_LIGHT_SHADOWS )
		directionalLightShadow = directionalLightShadows[ i ];
		directLight.color *= ( directLight.visible && receiveShadow ) ? getShadow( directionalShadowMap[ i ], directionalLightShadow.shadowMapSize, directionalLightShadow.shadowIntensity, directionalLightShadow.shadowBias, directionalLightShadow.shadowRadius, vDirectionalShadowCoord[ i ] ) : 1.0;
		#endif
		RE_Direct( directLight, geometryPosition, geometryNormal, geometryViewDir, geometryClearcoatNormal, material, reflectedLight );
	}
	#pragma unroll_loop_end
#endif
#if ( NUM_RECT_AREA_LIGHTS > 0 ) && defined( RE_Direct_RectArea )
	RectAreaLight rectAreaLight;
	#pragma unroll_loop_start
	for ( int i = 0; i < NUM_RECT_AREA_LIGHTS; i ++ ) {
		rectAreaLight = rectAreaLights[ i ];
		RE_Direct_RectArea( rectAreaLight, geometryPosition, geometryNormal, geometryViewDir, geometryClearcoatNormal, material, reflectedLight );
	}
	#pragma unroll_loop_end
#endif
#if defined( RE_IndirectDiffuse )
	vec3 iblIrradiance = vec3( 0.0 );
	vec3 irradiance = getAmbientLightIrradiance( ambientLightColor );
	#if defined( USE_LIGHT_PROBES )
		irradiance += getLightProbeIrradiance( lightProbe, geometryNormal );
	#endif
	#if ( NUM_HEMI_LIGHTS > 0 )
		#pragma unroll_loop_start
		for ( int i = 0; i < NUM_HEMI_LIGHTS; i ++ ) {
			irradiance += getHemisphereLightIrradiance( hemisphereLights[ i ], geometryNormal );
		}
		#pragma unroll_loop_end
	#endif
#endif
#if defined( RE_IndirectSpecular )
	vec3 radiance = vec3( 0.0 );
	vec3 clearcoatRadiance = vec3( 0.0 );
#endif`,Wv=`#if defined( RE_IndirectDiffuse )
	#ifdef USE_LIGHTMAP
		vec4 lightMapTexel = texture2D( lightMap, vLightMapUv );
		vec3 lightMapIrradiance = lightMapTexel.rgb * lightMapIntensity;
		irradiance += lightMapIrradiance;
	#endif
	#if defined( USE_ENVMAP ) && defined( STANDARD ) && defined( ENVMAP_TYPE_CUBE_UV )
		iblIrradiance += getIBLIrradiance( geometryNormal );
	#endif
#endif
#if defined( USE_ENVMAP ) && defined( RE_IndirectSpecular )
	#ifdef USE_ANISOTROPY
		radiance += getIBLAnisotropyRadiance( geometryViewDir, geometryNormal, material.roughness, material.anisotropyB, material.anisotropy );
	#else
		radiance += getIBLRadiance( geometryViewDir, geometryNormal, material.roughness );
	#endif
	#ifdef USE_CLEARCOAT
		clearcoatRadiance += getIBLRadiance( geometryViewDir, geometryClearcoatNormal, material.clearcoatRoughness );
	#endif
#endif`,qv=`#if defined( RE_IndirectDiffuse )
	RE_IndirectDiffuse( irradiance, geometryPosition, geometryNormal, geometryViewDir, geometryClearcoatNormal, material, reflectedLight );
#endif
#if defined( RE_IndirectSpecular )
	RE_IndirectSpecular( radiance, iblIrradiance, clearcoatRadiance, geometryPosition, geometryNormal, geometryViewDir, geometryClearcoatNormal, material, reflectedLight );
#endif`,Xv=`#if defined( USE_LOGARITHMIC_DEPTH_BUFFER )
	gl_FragDepth = vIsPerspective == 0.0 ? gl_FragCoord.z : log2( vFragDepth ) * logDepthBufFC * 0.5;
#endif`,jv=`#if defined( USE_LOGARITHMIC_DEPTH_BUFFER )
	uniform float logDepthBufFC;
	varying float vFragDepth;
	varying float vIsPerspective;
#endif`,Yv=`#ifdef USE_LOGARITHMIC_DEPTH_BUFFER
	varying float vFragDepth;
	varying float vIsPerspective;
#endif`,Zv=`#ifdef USE_LOGARITHMIC_DEPTH_BUFFER
	vFragDepth = 1.0 + gl_Position.w;
	vIsPerspective = float( isPerspectiveMatrix( projectionMatrix ) );
#endif`,Kv=`#ifdef USE_MAP
	vec4 sampledDiffuseColor = texture2D( map, vMapUv );
	#ifdef DECODE_VIDEO_TEXTURE
		sampledDiffuseColor = sRGBTransferEOTF( sampledDiffuseColor );
	#endif
	diffuseColor *= sampledDiffuseColor;
#endif`,Jv=`#ifdef USE_MAP
	uniform sampler2D map;
#endif`,Qv=`#if defined( USE_MAP ) || defined( USE_ALPHAMAP )
	#if defined( USE_POINTS_UV )
		vec2 uv = vUv;
	#else
		vec2 uv = ( uvTransform * vec3( gl_PointCoord.x, 1.0 - gl_PointCoord.y, 1 ) ).xy;
	#endif
#endif
#ifdef USE_MAP
	diffuseColor *= texture2D( map, uv );
#endif
#ifdef USE_ALPHAMAP
	diffuseColor.a *= texture2D( alphaMap, uv ).g;
#endif`,ex=`#if defined( USE_POINTS_UV )
	varying vec2 vUv;
#else
	#if defined( USE_MAP ) || defined( USE_ALPHAMAP )
		uniform mat3 uvTransform;
	#endif
#endif
#ifdef USE_MAP
	uniform sampler2D map;
#endif
#ifdef USE_ALPHAMAP
	uniform sampler2D alphaMap;
#endif`,tx=`float metalnessFactor = metalness;
#ifdef USE_METALNESSMAP
	vec4 texelMetalness = texture2D( metalnessMap, vMetalnessMapUv );
	metalnessFactor *= texelMetalness.b;
#endif`,nx=`#ifdef USE_METALNESSMAP
	uniform sampler2D metalnessMap;
#endif`,ix=`#ifdef USE_INSTANCING_MORPH
	float morphTargetInfluences[ MORPHTARGETS_COUNT ];
	float morphTargetBaseInfluence = texelFetch( morphTexture, ivec2( 0, gl_InstanceID ), 0 ).r;
	for ( int i = 0; i < MORPHTARGETS_COUNT; i ++ ) {
		morphTargetInfluences[i] =  texelFetch( morphTexture, ivec2( i + 1, gl_InstanceID ), 0 ).r;
	}
#endif`,rx=`#if defined( USE_MORPHCOLORS )
	vColor *= morphTargetBaseInfluence;
	for ( int i = 0; i < MORPHTARGETS_COUNT; i ++ ) {
		#if defined( USE_COLOR_ALPHA )
			if ( morphTargetInfluences[ i ] != 0.0 ) vColor += getMorph( gl_VertexID, i, 2 ) * morphTargetInfluences[ i ];
		#elif defined( USE_COLOR )
			if ( morphTargetInfluences[ i ] != 0.0 ) vColor += getMorph( gl_VertexID, i, 2 ).rgb * morphTargetInfluences[ i ];
		#endif
	}
#endif`,sx=`#ifdef USE_MORPHNORMALS
	objectNormal *= morphTargetBaseInfluence;
	for ( int i = 0; i < MORPHTARGETS_COUNT; i ++ ) {
		if ( morphTargetInfluences[ i ] != 0.0 ) objectNormal += getMorph( gl_VertexID, i, 1 ).xyz * morphTargetInfluences[ i ];
	}
#endif`,ax=`#ifdef USE_MORPHTARGETS
	#ifndef USE_INSTANCING_MORPH
		uniform float morphTargetBaseInfluence;
		uniform float morphTargetInfluences[ MORPHTARGETS_COUNT ];
	#endif
	uniform sampler2DArray morphTargetsTexture;
	uniform ivec2 morphTargetsTextureSize;
	vec4 getMorph( const in int vertexIndex, const in int morphTargetIndex, const in int offset ) {
		int texelIndex = vertexIndex * MORPHTARGETS_TEXTURE_STRIDE + offset;
		int y = texelIndex / morphTargetsTextureSize.x;
		int x = texelIndex - y * morphTargetsTextureSize.x;
		ivec3 morphUV = ivec3( x, y, morphTargetIndex );
		return texelFetch( morphTargetsTexture, morphUV, 0 );
	}
#endif`,ox=`#ifdef USE_MORPHTARGETS
	transformed *= morphTargetBaseInfluence;
	for ( int i = 0; i < MORPHTARGETS_COUNT; i ++ ) {
		if ( morphTargetInfluences[ i ] != 0.0 ) transformed += getMorph( gl_VertexID, i, 0 ).xyz * morphTargetInfluences[ i ];
	}
#endif`,cx=`float faceDirection = gl_FrontFacing ? 1.0 : - 1.0;
#ifdef FLAT_SHADED
	vec3 fdx = dFdx( vViewPosition );
	vec3 fdy = dFdy( vViewPosition );
	vec3 normal = normalize( cross( fdx, fdy ) );
#else
	vec3 normal = normalize( vNormal );
	#ifdef DOUBLE_SIDED
		normal *= faceDirection;
	#endif
#endif
#if defined( USE_NORMALMAP_TANGENTSPACE ) || defined( USE_CLEARCOAT_NORMALMAP ) || defined( USE_ANISOTROPY )
	#ifdef USE_TANGENT
		mat3 tbn = mat3( normalize( vTangent ), normalize( vBitangent ), normal );
	#else
		mat3 tbn = getTangentFrame( - vViewPosition, normal,
		#if defined( USE_NORMALMAP )
			vNormalMapUv
		#elif defined( USE_CLEARCOAT_NORMALMAP )
			vClearcoatNormalMapUv
		#else
			vUv
		#endif
		);
	#endif
	#if defined( DOUBLE_SIDED ) && ! defined( FLAT_SHADED )
		tbn[0] *= faceDirection;
		tbn[1] *= faceDirection;
	#endif
#endif
#ifdef USE_CLEARCOAT_NORMALMAP
	#ifdef USE_TANGENT
		mat3 tbn2 = mat3( normalize( vTangent ), normalize( vBitangent ), normal );
	#else
		mat3 tbn2 = getTangentFrame( - vViewPosition, normal, vClearcoatNormalMapUv );
	#endif
	#if defined( DOUBLE_SIDED ) && ! defined( FLAT_SHADED )
		tbn2[0] *= faceDirection;
		tbn2[1] *= faceDirection;
	#endif
#endif
vec3 nonPerturbedNormal = normal;`,lx=`#ifdef USE_NORMALMAP_OBJECTSPACE
	normal = texture2D( normalMap, vNormalMapUv ).xyz * 2.0 - 1.0;
	#ifdef FLIP_SIDED
		normal = - normal;
	#endif
	#ifdef DOUBLE_SIDED
		normal = normal * faceDirection;
	#endif
	normal = normalize( normalMatrix * normal );
#elif defined( USE_NORMALMAP_TANGENTSPACE )
	vec3 mapN = texture2D( normalMap, vNormalMapUv ).xyz * 2.0 - 1.0;
	mapN.xy *= normalScale;
	normal = normalize( tbn * mapN );
#elif defined( USE_BUMPMAP )
	normal = perturbNormalArb( - vViewPosition, normal, dHdxy_fwd(), faceDirection );
#endif`,ux=`#ifndef FLAT_SHADED
	varying vec3 vNormal;
	#ifdef USE_TANGENT
		varying vec3 vTangent;
		varying vec3 vBitangent;
	#endif
#endif`,hx=`#ifndef FLAT_SHADED
	varying vec3 vNormal;
	#ifdef USE_TANGENT
		varying vec3 vTangent;
		varying vec3 vBitangent;
	#endif
#endif`,dx=`#ifndef FLAT_SHADED
	vNormal = normalize( transformedNormal );
	#ifdef USE_TANGENT
		vTangent = normalize( transformedTangent );
		vBitangent = normalize( cross( vNormal, vTangent ) * tangent.w );
	#endif
#endif`,fx=`#ifdef USE_NORMALMAP
	uniform sampler2D normalMap;
	uniform vec2 normalScale;
#endif
#ifdef USE_NORMALMAP_OBJECTSPACE
	uniform mat3 normalMatrix;
#endif
#if ! defined ( USE_TANGENT ) && ( defined ( USE_NORMALMAP_TANGENTSPACE ) || defined ( USE_CLEARCOAT_NORMALMAP ) || defined( USE_ANISOTROPY ) )
	mat3 getTangentFrame( vec3 eye_pos, vec3 surf_norm, vec2 uv ) {
		vec3 q0 = dFdx( eye_pos.xyz );
		vec3 q1 = dFdy( eye_pos.xyz );
		vec2 st0 = dFdx( uv.st );
		vec2 st1 = dFdy( uv.st );
		vec3 N = surf_norm;
		vec3 q1perp = cross( q1, N );
		vec3 q0perp = cross( N, q0 );
		vec3 T = q1perp * st0.x + q0perp * st1.x;
		vec3 B = q1perp * st0.y + q0perp * st1.y;
		float det = max( dot( T, T ), dot( B, B ) );
		float scale = ( det == 0.0 ) ? 0.0 : inversesqrt( det );
		return mat3( T * scale, B * scale, N );
	}
#endif`,px=`#ifdef USE_CLEARCOAT
	vec3 clearcoatNormal = nonPerturbedNormal;
#endif`,mx=`#ifdef USE_CLEARCOAT_NORMALMAP
	vec3 clearcoatMapN = texture2D( clearcoatNormalMap, vClearcoatNormalMapUv ).xyz * 2.0 - 1.0;
	clearcoatMapN.xy *= clearcoatNormalScale;
	clearcoatNormal = normalize( tbn2 * clearcoatMapN );
#endif`,gx=`#ifdef USE_CLEARCOATMAP
	uniform sampler2D clearcoatMap;
#endif
#ifdef USE_CLEARCOAT_NORMALMAP
	uniform sampler2D clearcoatNormalMap;
	uniform vec2 clearcoatNormalScale;
#endif
#ifdef USE_CLEARCOAT_ROUGHNESSMAP
	uniform sampler2D clearcoatRoughnessMap;
#endif`,_x=`#ifdef USE_IRIDESCENCEMAP
	uniform sampler2D iridescenceMap;
#endif
#ifdef USE_IRIDESCENCE_THICKNESSMAP
	uniform sampler2D iridescenceThicknessMap;
#endif`,yx=`#ifdef OPAQUE
diffuseColor.a = 1.0;
#endif
#ifdef USE_TRANSMISSION
diffuseColor.a *= material.transmissionAlpha;
#endif
gl_FragColor = vec4( outgoingLight, diffuseColor.a );`,vx=`vec3 packNormalToRGB( const in vec3 normal ) {
	return normalize( normal ) * 0.5 + 0.5;
}
vec3 unpackRGBToNormal( const in vec3 rgb ) {
	return 2.0 * rgb.xyz - 1.0;
}
const float PackUpscale = 256. / 255.;const float UnpackDownscale = 255. / 256.;const float ShiftRight8 = 1. / 256.;
const float Inv255 = 1. / 255.;
const vec4 PackFactors = vec4( 1.0, 256.0, 256.0 * 256.0, 256.0 * 256.0 * 256.0 );
const vec2 UnpackFactors2 = vec2( UnpackDownscale, 1.0 / PackFactors.g );
const vec3 UnpackFactors3 = vec3( UnpackDownscale / PackFactors.rg, 1.0 / PackFactors.b );
const vec4 UnpackFactors4 = vec4( UnpackDownscale / PackFactors.rgb, 1.0 / PackFactors.a );
vec4 packDepthToRGBA( const in float v ) {
	if( v <= 0.0 )
		return vec4( 0., 0., 0., 0. );
	if( v >= 1.0 )
		return vec4( 1., 1., 1., 1. );
	float vuf;
	float af = modf( v * PackFactors.a, vuf );
	float bf = modf( vuf * ShiftRight8, vuf );
	float gf = modf( vuf * ShiftRight8, vuf );
	return vec4( vuf * Inv255, gf * PackUpscale, bf * PackUpscale, af );
}
vec3 packDepthToRGB( const in float v ) {
	if( v <= 0.0 )
		return vec3( 0., 0., 0. );
	if( v >= 1.0 )
		return vec3( 1., 1., 1. );
	float vuf;
	float bf = modf( v * PackFactors.b, vuf );
	float gf = modf( vuf * ShiftRight8, vuf );
	return vec3( vuf * Inv255, gf * PackUpscale, bf );
}
vec2 packDepthToRG( const in float v ) {
	if( v <= 0.0 )
		return vec2( 0., 0. );
	if( v >= 1.0 )
		return vec2( 1., 1. );
	float vuf;
	float gf = modf( v * 256., vuf );
	return vec2( vuf * Inv255, gf );
}
float unpackRGBAToDepth( const in vec4 v ) {
	return dot( v, UnpackFactors4 );
}
float unpackRGBToDepth( const in vec3 v ) {
	return dot( v, UnpackFactors3 );
}
float unpackRGToDepth( const in vec2 v ) {
	return v.r * UnpackFactors2.r + v.g * UnpackFactors2.g;
}
vec4 pack2HalfToRGBA( const in vec2 v ) {
	vec4 r = vec4( v.x, fract( v.x * 255.0 ), v.y, fract( v.y * 255.0 ) );
	return vec4( r.x - r.y / 255.0, r.y, r.z - r.w / 255.0, r.w );
}
vec2 unpackRGBATo2Half( const in vec4 v ) {
	return vec2( v.x + ( v.y / 255.0 ), v.z + ( v.w / 255.0 ) );
}
float viewZToOrthographicDepth( const in float viewZ, const in float near, const in float far ) {
	return ( viewZ + near ) / ( near - far );
}
float orthographicDepthToViewZ( const in float depth, const in float near, const in float far ) {
	return depth * ( near - far ) - near;
}
float viewZToPerspectiveDepth( const in float viewZ, const in float near, const in float far ) {
	return ( ( near + viewZ ) * far ) / ( ( far - near ) * viewZ );
}
float perspectiveDepthToViewZ( const in float depth, const in float near, const in float far ) {
	return ( near * far ) / ( ( far - near ) * depth - far );
}`,xx=`#ifdef PREMULTIPLIED_ALPHA
	gl_FragColor.rgb *= gl_FragColor.a;
#endif`,Sx=`vec4 mvPosition = vec4( transformed, 1.0 );
#ifdef USE_BATCHING
	mvPosition = batchingMatrix * mvPosition;
#endif
#ifdef USE_INSTANCING
	mvPosition = instanceMatrix * mvPosition;
#endif
mvPosition = modelViewMatrix * mvPosition;
gl_Position = projectionMatrix * mvPosition;`,bx=`#ifdef DITHERING
	gl_FragColor.rgb = dithering( gl_FragColor.rgb );
#endif`,Mx=`#ifdef DITHERING
	vec3 dithering( vec3 color ) {
		float grid_position = rand( gl_FragCoord.xy );
		vec3 dither_shift_RGB = vec3( 0.25 / 255.0, -0.25 / 255.0, 0.25 / 255.0 );
		dither_shift_RGB = mix( 2.0 * dither_shift_RGB, -2.0 * dither_shift_RGB, grid_position );
		return color + dither_shift_RGB;
	}
#endif`,Ex=`float roughnessFactor = roughness;
#ifdef USE_ROUGHNESSMAP
	vec4 texelRoughness = texture2D( roughnessMap, vRoughnessMapUv );
	roughnessFactor *= texelRoughness.g;
#endif`,wx=`#ifdef USE_ROUGHNESSMAP
	uniform sampler2D roughnessMap;
#endif`,Tx=`#if NUM_SPOT_LIGHT_COORDS > 0
	varying vec4 vSpotLightCoord[ NUM_SPOT_LIGHT_COORDS ];
#endif
#if NUM_SPOT_LIGHT_MAPS > 0
	uniform sampler2D spotLightMap[ NUM_SPOT_LIGHT_MAPS ];
#endif
#ifdef USE_SHADOWMAP
	#if NUM_DIR_LIGHT_SHADOWS > 0
		uniform sampler2D directionalShadowMap[ NUM_DIR_LIGHT_SHADOWS ];
		varying vec4 vDirectionalShadowCoord[ NUM_DIR_LIGHT_SHADOWS ];
		struct DirectionalLightShadow {
			float shadowIntensity;
			float shadowBias;
			float shadowNormalBias;
			float shadowRadius;
			vec2 shadowMapSize;
		};
		uniform DirectionalLightShadow directionalLightShadows[ NUM_DIR_LIGHT_SHADOWS ];
	#endif
	#if NUM_SPOT_LIGHT_SHADOWS > 0
		uniform sampler2D spotShadowMap[ NUM_SPOT_LIGHT_SHADOWS ];
		struct SpotLightShadow {
			float shadowIntensity;
			float shadowBias;
			float shadowNormalBias;
			float shadowRadius;
			vec2 shadowMapSize;
		};
		uniform SpotLightShadow spotLightShadows[ NUM_SPOT_LIGHT_SHADOWS ];
	#endif
	#if NUM_POINT_LIGHT_SHADOWS > 0
		uniform sampler2D pointShadowMap[ NUM_POINT_LIGHT_SHADOWS ];
		varying vec4 vPointShadowCoord[ NUM_POINT_LIGHT_SHADOWS ];
		struct PointLightShadow {
			float shadowIntensity;
			float shadowBias;
			float shadowNormalBias;
			float shadowRadius;
			vec2 shadowMapSize;
			float shadowCameraNear;
			float shadowCameraFar;
		};
		uniform PointLightShadow pointLightShadows[ NUM_POINT_LIGHT_SHADOWS ];
	#endif
	float texture2DCompare( sampler2D depths, vec2 uv, float compare ) {
		float depth = unpackRGBAToDepth( texture2D( depths, uv ) );
		#ifdef USE_REVERSED_DEPTH_BUFFER
			return step( depth, compare );
		#else
			return step( compare, depth );
		#endif
	}
	vec2 texture2DDistribution( sampler2D shadow, vec2 uv ) {
		return unpackRGBATo2Half( texture2D( shadow, uv ) );
	}
	float VSMShadow( sampler2D shadow, vec2 uv, float compare ) {
		float occlusion = 1.0;
		vec2 distribution = texture2DDistribution( shadow, uv );
		#ifdef USE_REVERSED_DEPTH_BUFFER
			float hard_shadow = step( distribution.x, compare );
		#else
			float hard_shadow = step( compare, distribution.x );
		#endif
		if ( hard_shadow != 1.0 ) {
			float distance = compare - distribution.x;
			float variance = max( 0.00000, distribution.y * distribution.y );
			float softness_probability = variance / (variance + distance * distance );			softness_probability = clamp( ( softness_probability - 0.3 ) / ( 0.95 - 0.3 ), 0.0, 1.0 );			occlusion = clamp( max( hard_shadow, softness_probability ), 0.0, 1.0 );
		}
		return occlusion;
	}
	float getShadow( sampler2D shadowMap, vec2 shadowMapSize, float shadowIntensity, float shadowBias, float shadowRadius, vec4 shadowCoord ) {
		float shadow = 1.0;
		shadowCoord.xyz /= shadowCoord.w;
		shadowCoord.z += shadowBias;
		bool inFrustum = shadowCoord.x >= 0.0 && shadowCoord.x <= 1.0 && shadowCoord.y >= 0.0 && shadowCoord.y <= 1.0;
		bool frustumTest = inFrustum && shadowCoord.z <= 1.0;
		if ( frustumTest ) {
		#if defined( SHADOWMAP_TYPE_PCF )
			vec2 texelSize = vec2( 1.0 ) / shadowMapSize;
			float dx0 = - texelSize.x * shadowRadius;
			float dy0 = - texelSize.y * shadowRadius;
			float dx1 = + texelSize.x * shadowRadius;
			float dy1 = + texelSize.y * shadowRadius;
			float dx2 = dx0 / 2.0;
			float dy2 = dy0 / 2.0;
			float dx3 = dx1 / 2.0;
			float dy3 = dy1 / 2.0;
			shadow = (
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( dx0, dy0 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( 0.0, dy0 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( dx1, dy0 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( dx2, dy2 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( 0.0, dy2 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( dx3, dy2 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( dx0, 0.0 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( dx2, 0.0 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy, shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( dx3, 0.0 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( dx1, 0.0 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( dx2, dy3 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( 0.0, dy3 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( dx3, dy3 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( dx0, dy1 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( 0.0, dy1 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, shadowCoord.xy + vec2( dx1, dy1 ), shadowCoord.z )
			) * ( 1.0 / 17.0 );
		#elif defined( SHADOWMAP_TYPE_PCF_SOFT )
			vec2 texelSize = vec2( 1.0 ) / shadowMapSize;
			float dx = texelSize.x;
			float dy = texelSize.y;
			vec2 uv = shadowCoord.xy;
			vec2 f = fract( uv * shadowMapSize + 0.5 );
			uv -= f * texelSize;
			shadow = (
				texture2DCompare( shadowMap, uv, shadowCoord.z ) +
				texture2DCompare( shadowMap, uv + vec2( dx, 0.0 ), shadowCoord.z ) +
				texture2DCompare( shadowMap, uv + vec2( 0.0, dy ), shadowCoord.z ) +
				texture2DCompare( shadowMap, uv + texelSize, shadowCoord.z ) +
				mix( texture2DCompare( shadowMap, uv + vec2( -dx, 0.0 ), shadowCoord.z ),
					 texture2DCompare( shadowMap, uv + vec2( 2.0 * dx, 0.0 ), shadowCoord.z ),
					 f.x ) +
				mix( texture2DCompare( shadowMap, uv + vec2( -dx, dy ), shadowCoord.z ),
					 texture2DCompare( shadowMap, uv + vec2( 2.0 * dx, dy ), shadowCoord.z ),
					 f.x ) +
				mix( texture2DCompare( shadowMap, uv + vec2( 0.0, -dy ), shadowCoord.z ),
					 texture2DCompare( shadowMap, uv + vec2( 0.0, 2.0 * dy ), shadowCoord.z ),
					 f.y ) +
				mix( texture2DCompare( shadowMap, uv + vec2( dx, -dy ), shadowCoord.z ),
					 texture2DCompare( shadowMap, uv + vec2( dx, 2.0 * dy ), shadowCoord.z ),
					 f.y ) +
				mix( mix( texture2DCompare( shadowMap, uv + vec2( -dx, -dy ), shadowCoord.z ),
						  texture2DCompare( shadowMap, uv + vec2( 2.0 * dx, -dy ), shadowCoord.z ),
						  f.x ),
					 mix( texture2DCompare( shadowMap, uv + vec2( -dx, 2.0 * dy ), shadowCoord.z ),
						  texture2DCompare( shadowMap, uv + vec2( 2.0 * dx, 2.0 * dy ), shadowCoord.z ),
						  f.x ),
					 f.y )
			) * ( 1.0 / 9.0 );
		#elif defined( SHADOWMAP_TYPE_VSM )
			shadow = VSMShadow( shadowMap, shadowCoord.xy, shadowCoord.z );
		#else
			shadow = texture2DCompare( shadowMap, shadowCoord.xy, shadowCoord.z );
		#endif
		}
		return mix( 1.0, shadow, shadowIntensity );
	}
	vec2 cubeToUV( vec3 v, float texelSizeY ) {
		vec3 absV = abs( v );
		float scaleToCube = 1.0 / max( absV.x, max( absV.y, absV.z ) );
		absV *= scaleToCube;
		v *= scaleToCube * ( 1.0 - 2.0 * texelSizeY );
		vec2 planar = v.xy;
		float almostATexel = 1.5 * texelSizeY;
		float almostOne = 1.0 - almostATexel;
		if ( absV.z >= almostOne ) {
			if ( v.z > 0.0 )
				planar.x = 4.0 - v.x;
		} else if ( absV.x >= almostOne ) {
			float signX = sign( v.x );
			planar.x = v.z * signX + 2.0 * signX;
		} else if ( absV.y >= almostOne ) {
			float signY = sign( v.y );
			planar.x = v.x + 2.0 * signY + 2.0;
			planar.y = v.z * signY - 2.0;
		}
		return vec2( 0.125, 0.25 ) * planar + vec2( 0.375, 0.75 );
	}
	float getPointShadow( sampler2D shadowMap, vec2 shadowMapSize, float shadowIntensity, float shadowBias, float shadowRadius, vec4 shadowCoord, float shadowCameraNear, float shadowCameraFar ) {
		float shadow = 1.0;
		vec3 lightToPosition = shadowCoord.xyz;
		
		float lightToPositionLength = length( lightToPosition );
		if ( lightToPositionLength - shadowCameraFar <= 0.0 && lightToPositionLength - shadowCameraNear >= 0.0 ) {
			float dp = ( lightToPositionLength - shadowCameraNear ) / ( shadowCameraFar - shadowCameraNear );			dp += shadowBias;
			vec3 bd3D = normalize( lightToPosition );
			vec2 texelSize = vec2( 1.0 ) / ( shadowMapSize * vec2( 4.0, 2.0 ) );
			#if defined( SHADOWMAP_TYPE_PCF ) || defined( SHADOWMAP_TYPE_PCF_SOFT ) || defined( SHADOWMAP_TYPE_VSM )
				vec2 offset = vec2( - 1, 1 ) * shadowRadius * texelSize.y;
				shadow = (
					texture2DCompare( shadowMap, cubeToUV( bd3D + offset.xyy, texelSize.y ), dp ) +
					texture2DCompare( shadowMap, cubeToUV( bd3D + offset.yyy, texelSize.y ), dp ) +
					texture2DCompare( shadowMap, cubeToUV( bd3D + offset.xyx, texelSize.y ), dp ) +
					texture2DCompare( shadowMap, cubeToUV( bd3D + offset.yyx, texelSize.y ), dp ) +
					texture2DCompare( shadowMap, cubeToUV( bd3D, texelSize.y ), dp ) +
					texture2DCompare( shadowMap, cubeToUV( bd3D + offset.xxy, texelSize.y ), dp ) +
					texture2DCompare( shadowMap, cubeToUV( bd3D + offset.yxy, texelSize.y ), dp ) +
					texture2DCompare( shadowMap, cubeToUV( bd3D + offset.xxx, texelSize.y ), dp ) +
					texture2DCompare( shadowMap, cubeToUV( bd3D + offset.yxx, texelSize.y ), dp )
				) * ( 1.0 / 9.0 );
			#else
				shadow = texture2DCompare( shadowMap, cubeToUV( bd3D, texelSize.y ), dp );
			#endif
		}
		return mix( 1.0, shadow, shadowIntensity );
	}
#endif`,Ax=`#if NUM_SPOT_LIGHT_COORDS > 0
	uniform mat4 spotLightMatrix[ NUM_SPOT_LIGHT_COORDS ];
	varying vec4 vSpotLightCoord[ NUM_SPOT_LIGHT_COORDS ];
#endif
#ifdef USE_SHADOWMAP
	#if NUM_DIR_LIGHT_SHADOWS > 0
		uniform mat4 directionalShadowMatrix[ NUM_DIR_LIGHT_SHADOWS ];
		varying vec4 vDirectionalShadowCoord[ NUM_DIR_LIGHT_SHADOWS ];
		struct DirectionalLightShadow {
			float shadowIntensity;
			float shadowBias;
			float shadowNormalBias;
			float shadowRadius;
			vec2 shadowMapSize;
		};
		uniform DirectionalLightShadow directionalLightShadows[ NUM_DIR_LIGHT_SHADOWS ];
	#endif
	#if NUM_SPOT_LIGHT_SHADOWS > 0
		struct SpotLightShadow {
			float shadowIntensity;
			float shadowBias;
			float shadowNormalBias;
			float shadowRadius;
			vec2 shadowMapSize;
		};
		uniform SpotLightShadow spotLightShadows[ NUM_SPOT_LIGHT_SHADOWS ];
	#endif
	#if NUM_POINT_LIGHT_SHADOWS > 0
		uniform mat4 pointShadowMatrix[ NUM_POINT_LIGHT_SHADOWS ];
		varying vec4 vPointShadowCoord[ NUM_POINT_LIGHT_SHADOWS ];
		struct PointLightShadow {
			float shadowIntensity;
			float shadowBias;
			float shadowNormalBias;
			float shadowRadius;
			vec2 shadowMapSize;
			float shadowCameraNear;
			float shadowCameraFar;
		};
		uniform PointLightShadow pointLightShadows[ NUM_POINT_LIGHT_SHADOWS ];
	#endif
#endif`,Rx=`#if ( defined( USE_SHADOWMAP ) && ( NUM_DIR_LIGHT_SHADOWS > 0 || NUM_POINT_LIGHT_SHADOWS > 0 ) ) || ( NUM_SPOT_LIGHT_COORDS > 0 )
	vec3 shadowWorldNormal = inverseTransformDirection( transformedNormal, viewMatrix );
	vec4 shadowWorldPosition;
#endif
#if defined( USE_SHADOWMAP )
	#if NUM_DIR_LIGHT_SHADOWS > 0
		#pragma unroll_loop_start
		for ( int i = 0; i < NUM_DIR_LIGHT_SHADOWS; i ++ ) {
			shadowWorldPosition = worldPosition + vec4( shadowWorldNormal * directionalLightShadows[ i ].shadowNormalBias, 0 );
			vDirectionalShadowCoord[ i ] = directionalShadowMatrix[ i ] * shadowWorldPosition;
		}
		#pragma unroll_loop_end
	#endif
	#if NUM_POINT_LIGHT_SHADOWS > 0
		#pragma unroll_loop_start
		for ( int i = 0; i < NUM_POINT_LIGHT_SHADOWS; i ++ ) {
			shadowWorldPosition = worldPosition + vec4( shadowWorldNormal * pointLightShadows[ i ].shadowNormalBias, 0 );
			vPointShadowCoord[ i ] = pointShadowMatrix[ i ] * shadowWorldPosition;
		}
		#pragma unroll_loop_end
	#endif
#endif
#if NUM_SPOT_LIGHT_COORDS > 0
	#pragma unroll_loop_start
	for ( int i = 0; i < NUM_SPOT_LIGHT_COORDS; i ++ ) {
		shadowWorldPosition = worldPosition;
		#if ( defined( USE_SHADOWMAP ) && UNROLLED_LOOP_INDEX < NUM_SPOT_LIGHT_SHADOWS )
			shadowWorldPosition.xyz += shadowWorldNormal * spotLightShadows[ i ].shadowNormalBias;
		#endif
		vSpotLightCoord[ i ] = spotLightMatrix[ i ] * shadowWorldPosition;
	}
	#pragma unroll_loop_end
#endif`,Cx=`float getShadowMask() {
	float shadow = 1.0;
	#ifdef USE_SHADOWMAP
	#if NUM_DIR_LIGHT_SHADOWS > 0
	DirectionalLightShadow directionalLight;
	#pragma unroll_loop_start
	for ( int i = 0; i < NUM_DIR_LIGHT_SHADOWS; i ++ ) {
		directionalLight = directionalLightShadows[ i ];
		shadow *= receiveShadow ? getShadow( directionalShadowMap[ i ], directionalLight.shadowMapSize, directionalLight.shadowIntensity, directionalLight.shadowBias, directionalLight.shadowRadius, vDirectionalShadowCoord[ i ] ) : 1.0;
	}
	#pragma unroll_loop_end
	#endif
	#if NUM_SPOT_LIGHT_SHADOWS > 0
	SpotLightShadow spotLight;
	#pragma unroll_loop_start
	for ( int i = 0; i < NUM_SPOT_LIGHT_SHADOWS; i ++ ) {
		spotLight = spotLightShadows[ i ];
		shadow *= receiveShadow ? getShadow( spotShadowMap[ i ], spotLight.shadowMapSize, spotLight.shadowIntensity, spotLight.shadowBias, spotLight.shadowRadius, vSpotLightCoord[ i ] ) : 1.0;
	}
	#pragma unroll_loop_end
	#endif
	#if NUM_POINT_LIGHT_SHADOWS > 0
	PointLightShadow pointLight;
	#pragma unroll_loop_start
	for ( int i = 0; i < NUM_POINT_LIGHT_SHADOWS; i ++ ) {
		pointLight = pointLightShadows[ i ];
		shadow *= receiveShadow ? getPointShadow( pointShadowMap[ i ], pointLight.shadowMapSize, pointLight.shadowIntensity, pointLight.shadowBias, pointLight.shadowRadius, vPointShadowCoord[ i ], pointLight.shadowCameraNear, pointLight.shadowCameraFar ) : 1.0;
	}
	#pragma unroll_loop_end
	#endif
	#endif
	return shadow;
}`,Px=`#ifdef USE_SKINNING
	mat4 boneMatX = getBoneMatrix( skinIndex.x );
	mat4 boneMatY = getBoneMatrix( skinIndex.y );
	mat4 boneMatZ = getBoneMatrix( skinIndex.z );
	mat4 boneMatW = getBoneMatrix( skinIndex.w );
#endif`,Ix=`#ifdef USE_SKINNING
	uniform mat4 bindMatrix;
	uniform mat4 bindMatrixInverse;
	uniform highp sampler2D boneTexture;
	mat4 getBoneMatrix( const in float i ) {
		int size = textureSize( boneTexture, 0 ).x;
		int j = int( i ) * 4;
		int x = j % size;
		int y = j / size;
		vec4 v1 = texelFetch( boneTexture, ivec2( x, y ), 0 );
		vec4 v2 = texelFetch( boneTexture, ivec2( x + 1, y ), 0 );
		vec4 v3 = texelFetch( boneTexture, ivec2( x + 2, y ), 0 );
		vec4 v4 = texelFetch( boneTexture, ivec2( x + 3, y ), 0 );
		return mat4( v1, v2, v3, v4 );
	}
#endif`,Dx=`#ifdef USE_SKINNING
	vec4 skinVertex = bindMatrix * vec4( transformed, 1.0 );
	vec4 skinned = vec4( 0.0 );
	skinned += boneMatX * skinVertex * skinWeight.x;
	skinned += boneMatY * skinVertex * skinWeight.y;
	skinned += boneMatZ * skinVertex * skinWeight.z;
	skinned += boneMatW * skinVertex * skinWeight.w;
	transformed = ( bindMatrixInverse * skinned ).xyz;
#endif`,Nx=`#ifdef USE_SKINNING
	mat4 skinMatrix = mat4( 0.0 );
	skinMatrix += skinWeight.x * boneMatX;
	skinMatrix += skinWeight.y * boneMatY;
	skinMatrix += skinWeight.z * boneMatZ;
	skinMatrix += skinWeight.w * boneMatW;
	skinMatrix = bindMatrixInverse * skinMatrix * bindMatrix;
	objectNormal = vec4( skinMatrix * vec4( objectNormal, 0.0 ) ).xyz;
	#ifdef USE_TANGENT
		objectTangent = vec4( skinMatrix * vec4( objectTangent, 0.0 ) ).xyz;
	#endif
#endif`,Lx=`float specularStrength;
#ifdef USE_SPECULARMAP
	vec4 texelSpecular = texture2D( specularMap, vSpecularMapUv );
	specularStrength = texelSpecular.r;
#else
	specularStrength = 1.0;
#endif`,Ux=`#ifdef USE_SPECULARMAP
	uniform sampler2D specularMap;
#endif`,Ox=`#if defined( TONE_MAPPING )
	gl_FragColor.rgb = toneMapping( gl_FragColor.rgb );
#endif`,Fx=`#ifndef saturate
#define saturate( a ) clamp( a, 0.0, 1.0 )
#endif
uniform float toneMappingExposure;
vec3 LinearToneMapping( vec3 color ) {
	return saturate( toneMappingExposure * color );
}
vec3 ReinhardToneMapping( vec3 color ) {
	color *= toneMappingExposure;
	return saturate( color / ( vec3( 1.0 ) + color ) );
}
vec3 CineonToneMapping( vec3 color ) {
	color *= toneMappingExposure;
	color = max( vec3( 0.0 ), color - 0.004 );
	return pow( ( color * ( 6.2 * color + 0.5 ) ) / ( color * ( 6.2 * color + 1.7 ) + 0.06 ), vec3( 2.2 ) );
}
vec3 RRTAndODTFit( vec3 v ) {
	vec3 a = v * ( v + 0.0245786 ) - 0.000090537;
	vec3 b = v * ( 0.983729 * v + 0.4329510 ) + 0.238081;
	return a / b;
}
vec3 ACESFilmicToneMapping( vec3 color ) {
	const mat3 ACESInputMat = mat3(
		vec3( 0.59719, 0.07600, 0.02840 ),		vec3( 0.35458, 0.90834, 0.13383 ),
		vec3( 0.04823, 0.01566, 0.83777 )
	);
	const mat3 ACESOutputMat = mat3(
		vec3(  1.60475, -0.10208, -0.00327 ),		vec3( -0.53108,  1.10813, -0.07276 ),
		vec3( -0.07367, -0.00605,  1.07602 )
	);
	color *= toneMappingExposure / 0.6;
	color = ACESInputMat * color;
	color = RRTAndODTFit( color );
	color = ACESOutputMat * color;
	return saturate( color );
}
const mat3 LINEAR_REC2020_TO_LINEAR_SRGB = mat3(
	vec3( 1.6605, - 0.1246, - 0.0182 ),
	vec3( - 0.5876, 1.1329, - 0.1006 ),
	vec3( - 0.0728, - 0.0083, 1.1187 )
);
const mat3 LINEAR_SRGB_TO_LINEAR_REC2020 = mat3(
	vec3( 0.6274, 0.0691, 0.0164 ),
	vec3( 0.3293, 0.9195, 0.0880 ),
	vec3( 0.0433, 0.0113, 0.8956 )
);
vec3 agxDefaultContrastApprox( vec3 x ) {
	vec3 x2 = x * x;
	vec3 x4 = x2 * x2;
	return + 15.5 * x4 * x2
		- 40.14 * x4 * x
		+ 31.96 * x4
		- 6.868 * x2 * x
		+ 0.4298 * x2
		+ 0.1191 * x
		- 0.00232;
}
vec3 AgXToneMapping( vec3 color ) {
	const mat3 AgXInsetMatrix = mat3(
		vec3( 0.856627153315983, 0.137318972929847, 0.11189821299995 ),
		vec3( 0.0951212405381588, 0.761241990602591, 0.0767994186031903 ),
		vec3( 0.0482516061458583, 0.101439036467562, 0.811302368396859 )
	);
	const mat3 AgXOutsetMatrix = mat3(
		vec3( 1.1271005818144368, - 0.1413297634984383, - 0.14132976349843826 ),
		vec3( - 0.11060664309660323, 1.157823702216272, - 0.11060664309660294 ),
		vec3( - 0.016493938717834573, - 0.016493938717834257, 1.2519364065950405 )
	);
	const float AgxMinEv = - 12.47393;	const float AgxMaxEv = 4.026069;
	color *= toneMappingExposure;
	color = LINEAR_SRGB_TO_LINEAR_REC2020 * color;
	color = AgXInsetMatrix * color;
	color = max( color, 1e-10 );	color = log2( color );
	color = ( color - AgxMinEv ) / ( AgxMaxEv - AgxMinEv );
	color = clamp( color, 0.0, 1.0 );
	color = agxDefaultContrastApprox( color );
	color = AgXOutsetMatrix * color;
	color = pow( max( vec3( 0.0 ), color ), vec3( 2.2 ) );
	color = LINEAR_REC2020_TO_LINEAR_SRGB * color;
	color = clamp( color, 0.0, 1.0 );
	return color;
}
vec3 NeutralToneMapping( vec3 color ) {
	const float StartCompression = 0.8 - 0.04;
	const float Desaturation = 0.15;
	color *= toneMappingExposure;
	float x = min( color.r, min( color.g, color.b ) );
	float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
	color -= offset;
	float peak = max( color.r, max( color.g, color.b ) );
	if ( peak < StartCompression ) return color;
	float d = 1. - StartCompression;
	float newPeak = 1. - d * d / ( peak + d - StartCompression );
	color *= newPeak / peak;
	float g = 1. - 1. / ( Desaturation * ( peak - newPeak ) + 1. );
	return mix( color, vec3( newPeak ), g );
}
vec3 CustomToneMapping( vec3 color ) { return color; }`,Bx=`#ifdef USE_TRANSMISSION
	material.transmission = transmission;
	material.transmissionAlpha = 1.0;
	material.thickness = thickness;
	material.attenuationDistance = attenuationDistance;
	material.attenuationColor = attenuationColor;
	#ifdef USE_TRANSMISSIONMAP
		material.transmission *= texture2D( transmissionMap, vTransmissionMapUv ).r;
	#endif
	#ifdef USE_THICKNESSMAP
		material.thickness *= texture2D( thicknessMap, vThicknessMapUv ).g;
	#endif
	vec3 pos = vWorldPosition;
	vec3 v = normalize( cameraPosition - pos );
	vec3 n = inverseTransformDirection( normal, viewMatrix );
	vec4 transmitted = getIBLVolumeRefraction(
		n, v, material.roughness, material.diffuseColor, material.specularColor, material.specularF90,
		pos, modelMatrix, viewMatrix, projectionMatrix, material.dispersion, material.ior, material.thickness,
		material.attenuationColor, material.attenuationDistance );
	material.transmissionAlpha = mix( material.transmissionAlpha, transmitted.a, material.transmission );
	totalDiffuse = mix( totalDiffuse, transmitted.rgb, material.transmission );
#endif`,kx=`#ifdef USE_TRANSMISSION
	uniform float transmission;
	uniform float thickness;
	uniform float attenuationDistance;
	uniform vec3 attenuationColor;
	#ifdef USE_TRANSMISSIONMAP
		uniform sampler2D transmissionMap;
	#endif
	#ifdef USE_THICKNESSMAP
		uniform sampler2D thicknessMap;
	#endif
	uniform vec2 transmissionSamplerSize;
	uniform sampler2D transmissionSamplerMap;
	uniform mat4 modelMatrix;
	uniform mat4 projectionMatrix;
	varying vec3 vWorldPosition;
	float w0( float a ) {
		return ( 1.0 / 6.0 ) * ( a * ( a * ( - a + 3.0 ) - 3.0 ) + 1.0 );
	}
	float w1( float a ) {
		return ( 1.0 / 6.0 ) * ( a *  a * ( 3.0 * a - 6.0 ) + 4.0 );
	}
	float w2( float a ){
		return ( 1.0 / 6.0 ) * ( a * ( a * ( - 3.0 * a + 3.0 ) + 3.0 ) + 1.0 );
	}
	float w3( float a ) {
		return ( 1.0 / 6.0 ) * ( a * a * a );
	}
	float g0( float a ) {
		return w0( a ) + w1( a );
	}
	float g1( float a ) {
		return w2( a ) + w3( a );
	}
	float h0( float a ) {
		return - 1.0 + w1( a ) / ( w0( a ) + w1( a ) );
	}
	float h1( float a ) {
		return 1.0 + w3( a ) / ( w2( a ) + w3( a ) );
	}
	vec4 bicubic( sampler2D tex, vec2 uv, vec4 texelSize, float lod ) {
		uv = uv * texelSize.zw + 0.5;
		vec2 iuv = floor( uv );
		vec2 fuv = fract( uv );
		float g0x = g0( fuv.x );
		float g1x = g1( fuv.x );
		float h0x = h0( fuv.x );
		float h1x = h1( fuv.x );
		float h0y = h0( fuv.y );
		float h1y = h1( fuv.y );
		vec2 p0 = ( vec2( iuv.x + h0x, iuv.y + h0y ) - 0.5 ) * texelSize.xy;
		vec2 p1 = ( vec2( iuv.x + h1x, iuv.y + h0y ) - 0.5 ) * texelSize.xy;
		vec2 p2 = ( vec2( iuv.x + h0x, iuv.y + h1y ) - 0.5 ) * texelSize.xy;
		vec2 p3 = ( vec2( iuv.x + h1x, iuv.y + h1y ) - 0.5 ) * texelSize.xy;
		return g0( fuv.y ) * ( g0x * textureLod( tex, p0, lod ) + g1x * textureLod( tex, p1, lod ) ) +
			g1( fuv.y ) * ( g0x * textureLod( tex, p2, lod ) + g1x * textureLod( tex, p3, lod ) );
	}
	vec4 textureBicubic( sampler2D sampler, vec2 uv, float lod ) {
		vec2 fLodSize = vec2( textureSize( sampler, int( lod ) ) );
		vec2 cLodSize = vec2( textureSize( sampler, int( lod + 1.0 ) ) );
		vec2 fLodSizeInv = 1.0 / fLodSize;
		vec2 cLodSizeInv = 1.0 / cLodSize;
		vec4 fSample = bicubic( sampler, uv, vec4( fLodSizeInv, fLodSize ), floor( lod ) );
		vec4 cSample = bicubic( sampler, uv, vec4( cLodSizeInv, cLodSize ), ceil( lod ) );
		return mix( fSample, cSample, fract( lod ) );
	}
	vec3 getVolumeTransmissionRay( const in vec3 n, const in vec3 v, const in float thickness, const in float ior, const in mat4 modelMatrix ) {
		vec3 refractionVector = refract( - v, normalize( n ), 1.0 / ior );
		vec3 modelScale;
		modelScale.x = length( vec3( modelMatrix[ 0 ].xyz ) );
		modelScale.y = length( vec3( modelMatrix[ 1 ].xyz ) );
		modelScale.z = length( vec3( modelMatrix[ 2 ].xyz ) );
		return normalize( refractionVector ) * thickness * modelScale;
	}
	float applyIorToRoughness( const in float roughness, const in float ior ) {
		return roughness * clamp( ior * 2.0 - 2.0, 0.0, 1.0 );
	}
	vec4 getTransmissionSample( const in vec2 fragCoord, const in float roughness, const in float ior ) {
		float lod = log2( transmissionSamplerSize.x ) * applyIorToRoughness( roughness, ior );
		return textureBicubic( transmissionSamplerMap, fragCoord.xy, lod );
	}
	vec3 volumeAttenuation( const in float transmissionDistance, const in vec3 attenuationColor, const in float attenuationDistance ) {
		if ( isinf( attenuationDistance ) ) {
			return vec3( 1.0 );
		} else {
			vec3 attenuationCoefficient = -log( attenuationColor ) / attenuationDistance;
			vec3 transmittance = exp( - attenuationCoefficient * transmissionDistance );			return transmittance;
		}
	}
	vec4 getIBLVolumeRefraction( const in vec3 n, const in vec3 v, const in float roughness, const in vec3 diffuseColor,
		const in vec3 specularColor, const in float specularF90, const in vec3 position, const in mat4 modelMatrix,
		const in mat4 viewMatrix, const in mat4 projMatrix, const in float dispersion, const in float ior, const in float thickness,
		const in vec3 attenuationColor, const in float attenuationDistance ) {
		vec4 transmittedLight;
		vec3 transmittance;
		#ifdef USE_DISPERSION
			float halfSpread = ( ior - 1.0 ) * 0.025 * dispersion;
			vec3 iors = vec3( ior - halfSpread, ior, ior + halfSpread );
			for ( int i = 0; i < 3; i ++ ) {
				vec3 transmissionRay = getVolumeTransmissionRay( n, v, thickness, iors[ i ], modelMatrix );
				vec3 refractedRayExit = position + transmissionRay;
				vec4 ndcPos = projMatrix * viewMatrix * vec4( refractedRayExit, 1.0 );
				vec2 refractionCoords = ndcPos.xy / ndcPos.w;
				refractionCoords += 1.0;
				refractionCoords /= 2.0;
				vec4 transmissionSample = getTransmissionSample( refractionCoords, roughness, iors[ i ] );
				transmittedLight[ i ] = transmissionSample[ i ];
				transmittedLight.a += transmissionSample.a;
				transmittance[ i ] = diffuseColor[ i ] * volumeAttenuation( length( transmissionRay ), attenuationColor, attenuationDistance )[ i ];
			}
			transmittedLight.a /= 3.0;
		#else
			vec3 transmissionRay = getVolumeTransmissionRay( n, v, thickness, ior, modelMatrix );
			vec3 refractedRayExit = position + transmissionRay;
			vec4 ndcPos = projMatrix * viewMatrix * vec4( refractedRayExit, 1.0 );
			vec2 refractionCoords = ndcPos.xy / ndcPos.w;
			refractionCoords += 1.0;
			refractionCoords /= 2.0;
			transmittedLight = getTransmissionSample( refractionCoords, roughness, ior );
			transmittance = diffuseColor * volumeAttenuation( length( transmissionRay ), attenuationColor, attenuationDistance );
		#endif
		vec3 attenuatedColor = transmittance * transmittedLight.rgb;
		vec3 F = EnvironmentBRDF( n, v, specularColor, specularF90, roughness );
		float transmittanceFactor = ( transmittance.r + transmittance.g + transmittance.b ) / 3.0;
		return vec4( ( 1.0 - F ) * attenuatedColor, 1.0 - ( 1.0 - transmittedLight.a ) * transmittanceFactor );
	}
#endif`,zx=`#if defined( USE_UV ) || defined( USE_ANISOTROPY )
	varying vec2 vUv;
#endif
#ifdef USE_MAP
	varying vec2 vMapUv;
#endif
#ifdef USE_ALPHAMAP
	varying vec2 vAlphaMapUv;
#endif
#ifdef USE_LIGHTMAP
	varying vec2 vLightMapUv;
#endif
#ifdef USE_AOMAP
	varying vec2 vAoMapUv;
#endif
#ifdef USE_BUMPMAP
	varying vec2 vBumpMapUv;
#endif
#ifdef USE_NORMALMAP
	varying vec2 vNormalMapUv;
#endif
#ifdef USE_EMISSIVEMAP
	varying vec2 vEmissiveMapUv;
#endif
#ifdef USE_METALNESSMAP
	varying vec2 vMetalnessMapUv;
#endif
#ifdef USE_ROUGHNESSMAP
	varying vec2 vRoughnessMapUv;
#endif
#ifdef USE_ANISOTROPYMAP
	varying vec2 vAnisotropyMapUv;
#endif
#ifdef USE_CLEARCOATMAP
	varying vec2 vClearcoatMapUv;
#endif
#ifdef USE_CLEARCOAT_NORMALMAP
	varying vec2 vClearcoatNormalMapUv;
#endif
#ifdef USE_CLEARCOAT_ROUGHNESSMAP
	varying vec2 vClearcoatRoughnessMapUv;
#endif
#ifdef USE_IRIDESCENCEMAP
	varying vec2 vIridescenceMapUv;
#endif
#ifdef USE_IRIDESCENCE_THICKNESSMAP
	varying vec2 vIridescenceThicknessMapUv;
#endif
#ifdef USE_SHEEN_COLORMAP
	varying vec2 vSheenColorMapUv;
#endif
#ifdef USE_SHEEN_ROUGHNESSMAP
	varying vec2 vSheenRoughnessMapUv;
#endif
#ifdef USE_SPECULARMAP
	varying vec2 vSpecularMapUv;
#endif
#ifdef USE_SPECULAR_COLORMAP
	varying vec2 vSpecularColorMapUv;
#endif
#ifdef USE_SPECULAR_INTENSITYMAP
	varying vec2 vSpecularIntensityMapUv;
#endif
#ifdef USE_TRANSMISSIONMAP
	uniform mat3 transmissionMapTransform;
	varying vec2 vTransmissionMapUv;
#endif
#ifdef USE_THICKNESSMAP
	uniform mat3 thicknessMapTransform;
	varying vec2 vThicknessMapUv;
#endif`,Vx=`#if defined( USE_UV ) || defined( USE_ANISOTROPY )
	varying vec2 vUv;
#endif
#ifdef USE_MAP
	uniform mat3 mapTransform;
	varying vec2 vMapUv;
#endif
#ifdef USE_ALPHAMAP
	uniform mat3 alphaMapTransform;
	varying vec2 vAlphaMapUv;
#endif
#ifdef USE_LIGHTMAP
	uniform mat3 lightMapTransform;
	varying vec2 vLightMapUv;
#endif
#ifdef USE_AOMAP
	uniform mat3 aoMapTransform;
	varying vec2 vAoMapUv;
#endif
#ifdef USE_BUMPMAP
	uniform mat3 bumpMapTransform;
	varying vec2 vBumpMapUv;
#endif
#ifdef USE_NORMALMAP
	uniform mat3 normalMapTransform;
	varying vec2 vNormalMapUv;
#endif
#ifdef USE_DISPLACEMENTMAP
	uniform mat3 displacementMapTransform;
	varying vec2 vDisplacementMapUv;
#endif
#ifdef USE_EMISSIVEMAP
	uniform mat3 emissiveMapTransform;
	varying vec2 vEmissiveMapUv;
#endif
#ifdef USE_METALNESSMAP
	uniform mat3 metalnessMapTransform;
	varying vec2 vMetalnessMapUv;
#endif
#ifdef USE_ROUGHNESSMAP
	uniform mat3 roughnessMapTransform;
	varying vec2 vRoughnessMapUv;
#endif
#ifdef USE_ANISOTROPYMAP
	uniform mat3 anisotropyMapTransform;
	varying vec2 vAnisotropyMapUv;
#endif
#ifdef USE_CLEARCOATMAP
	uniform mat3 clearcoatMapTransform;
	varying vec2 vClearcoatMapUv;
#endif
#ifdef USE_CLEARCOAT_NORMALMAP
	uniform mat3 clearcoatNormalMapTransform;
	varying vec2 vClearcoatNormalMapUv;
#endif
#ifdef USE_CLEARCOAT_ROUGHNESSMAP
	uniform mat3 clearcoatRoughnessMapTransform;
	varying vec2 vClearcoatRoughnessMapUv;
#endif
#ifdef USE_SHEEN_COLORMAP
	uniform mat3 sheenColorMapTransform;
	varying vec2 vSheenColorMapUv;
#endif
#ifdef USE_SHEEN_ROUGHNESSMAP
	uniform mat3 sheenRoughnessMapTransform;
	varying vec2 vSheenRoughnessMapUv;
#endif
#ifdef USE_IRIDESCENCEMAP
	uniform mat3 iridescenceMapTransform;
	varying vec2 vIridescenceMapUv;
#endif
#ifdef USE_IRIDESCENCE_THICKNESSMAP
	uniform mat3 iridescenceThicknessMapTransform;
	varying vec2 vIridescenceThicknessMapUv;
#endif
#ifdef USE_SPECULARMAP
	uniform mat3 specularMapTransform;
	varying vec2 vSpecularMapUv;
#endif
#ifdef USE_SPECULAR_COLORMAP
	uniform mat3 specularColorMapTransform;
	varying vec2 vSpecularColorMapUv;
#endif
#ifdef USE_SPECULAR_INTENSITYMAP
	uniform mat3 specularIntensityMapTransform;
	varying vec2 vSpecularIntensityMapUv;
#endif
#ifdef USE_TRANSMISSIONMAP
	uniform mat3 transmissionMapTransform;
	varying vec2 vTransmissionMapUv;
#endif
#ifdef USE_THICKNESSMAP
	uniform mat3 thicknessMapTransform;
	varying vec2 vThicknessMapUv;
#endif`,$x=`#if defined( USE_UV ) || defined( USE_ANISOTROPY )
	vUv = vec3( uv, 1 ).xy;
#endif
#ifdef USE_MAP
	vMapUv = ( mapTransform * vec3( MAP_UV, 1 ) ).xy;
#endif
#ifdef USE_ALPHAMAP
	vAlphaMapUv = ( alphaMapTransform * vec3( ALPHAMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_LIGHTMAP
	vLightMapUv = ( lightMapTransform * vec3( LIGHTMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_AOMAP
	vAoMapUv = ( aoMapTransform * vec3( AOMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_BUMPMAP
	vBumpMapUv = ( bumpMapTransform * vec3( BUMPMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_NORMALMAP
	vNormalMapUv = ( normalMapTransform * vec3( NORMALMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_DISPLACEMENTMAP
	vDisplacementMapUv = ( displacementMapTransform * vec3( DISPLACEMENTMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_EMISSIVEMAP
	vEmissiveMapUv = ( emissiveMapTransform * vec3( EMISSIVEMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_METALNESSMAP
	vMetalnessMapUv = ( metalnessMapTransform * vec3( METALNESSMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_ROUGHNESSMAP
	vRoughnessMapUv = ( roughnessMapTransform * vec3( ROUGHNESSMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_ANISOTROPYMAP
	vAnisotropyMapUv = ( anisotropyMapTransform * vec3( ANISOTROPYMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_CLEARCOATMAP
	vClearcoatMapUv = ( clearcoatMapTransform * vec3( CLEARCOATMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_CLEARCOAT_NORMALMAP
	vClearcoatNormalMapUv = ( clearcoatNormalMapTransform * vec3( CLEARCOAT_NORMALMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_CLEARCOAT_ROUGHNESSMAP
	vClearcoatRoughnessMapUv = ( clearcoatRoughnessMapTransform * vec3( CLEARCOAT_ROUGHNESSMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_IRIDESCENCEMAP
	vIridescenceMapUv = ( iridescenceMapTransform * vec3( IRIDESCENCEMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_IRIDESCENCE_THICKNESSMAP
	vIridescenceThicknessMapUv = ( iridescenceThicknessMapTransform * vec3( IRIDESCENCE_THICKNESSMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_SHEEN_COLORMAP
	vSheenColorMapUv = ( sheenColorMapTransform * vec3( SHEEN_COLORMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_SHEEN_ROUGHNESSMAP
	vSheenRoughnessMapUv = ( sheenRoughnessMapTransform * vec3( SHEEN_ROUGHNESSMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_SPECULARMAP
	vSpecularMapUv = ( specularMapTransform * vec3( SPECULARMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_SPECULAR_COLORMAP
	vSpecularColorMapUv = ( specularColorMapTransform * vec3( SPECULAR_COLORMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_SPECULAR_INTENSITYMAP
	vSpecularIntensityMapUv = ( specularIntensityMapTransform * vec3( SPECULAR_INTENSITYMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_TRANSMISSIONMAP
	vTransmissionMapUv = ( transmissionMapTransform * vec3( TRANSMISSIONMAP_UV, 1 ) ).xy;
#endif
#ifdef USE_THICKNESSMAP
	vThicknessMapUv = ( thicknessMapTransform * vec3( THICKNESSMAP_UV, 1 ) ).xy;
#endif`,Hx=`#if defined( USE_ENVMAP ) || defined( DISTANCE ) || defined ( USE_SHADOWMAP ) || defined ( USE_TRANSMISSION ) || NUM_SPOT_LIGHT_COORDS > 0
	vec4 worldPosition = vec4( transformed, 1.0 );
	#ifdef USE_BATCHING
		worldPosition = batchingMatrix * worldPosition;
	#endif
	#ifdef USE_INSTANCING
		worldPosition = instanceMatrix * worldPosition;
	#endif
	worldPosition = modelMatrix * worldPosition;
#endif`,Gx=`varying vec2 vUv;
uniform mat3 uvTransform;
void main() {
	vUv = ( uvTransform * vec3( uv, 1 ) ).xy;
	gl_Position = vec4( position.xy, 1.0, 1.0 );
}`,Wx=`uniform sampler2D t2D;
uniform float backgroundIntensity;
varying vec2 vUv;
void main() {
	vec4 texColor = texture2D( t2D, vUv );
	#ifdef DECODE_VIDEO_TEXTURE
		texColor = vec4( mix( pow( texColor.rgb * 0.9478672986 + vec3( 0.0521327014 ), vec3( 2.4 ) ), texColor.rgb * 0.0773993808, vec3( lessThanEqual( texColor.rgb, vec3( 0.04045 ) ) ) ), texColor.w );
	#endif
	texColor.rgb *= backgroundIntensity;
	gl_FragColor = texColor;
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
}`,qx=`varying vec3 vWorldDirection;
#include <common>
void main() {
	vWorldDirection = transformDirection( position, modelMatrix );
	#include <begin_vertex>
	#include <project_vertex>
	gl_Position.z = gl_Position.w;
}`,Xx=`#ifdef ENVMAP_TYPE_CUBE
	uniform samplerCube envMap;
#elif defined( ENVMAP_TYPE_CUBE_UV )
	uniform sampler2D envMap;
#endif
uniform float flipEnvMap;
uniform float backgroundBlurriness;
uniform float backgroundIntensity;
uniform mat3 backgroundRotation;
varying vec3 vWorldDirection;
#include <cube_uv_reflection_fragment>
void main() {
	#ifdef ENVMAP_TYPE_CUBE
		vec4 texColor = textureCube( envMap, backgroundRotation * vec3( flipEnvMap * vWorldDirection.x, vWorldDirection.yz ) );
	#elif defined( ENVMAP_TYPE_CUBE_UV )
		vec4 texColor = textureCubeUV( envMap, backgroundRotation * vWorldDirection, backgroundBlurriness );
	#else
		vec4 texColor = vec4( 0.0, 0.0, 0.0, 1.0 );
	#endif
	texColor.rgb *= backgroundIntensity;
	gl_FragColor = texColor;
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
}`,jx=`varying vec3 vWorldDirection;
#include <common>
void main() {
	vWorldDirection = transformDirection( position, modelMatrix );
	#include <begin_vertex>
	#include <project_vertex>
	gl_Position.z = gl_Position.w;
}`,Yx=`uniform samplerCube tCube;
uniform float tFlip;
uniform float opacity;
varying vec3 vWorldDirection;
void main() {
	vec4 texColor = textureCube( tCube, vec3( tFlip * vWorldDirection.x, vWorldDirection.yz ) );
	gl_FragColor = texColor;
	gl_FragColor.a *= opacity;
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
}`,Zx=`#include <common>
#include <batching_pars_vertex>
#include <uv_pars_vertex>
#include <displacementmap_pars_vertex>
#include <morphtarget_pars_vertex>
#include <skinning_pars_vertex>
#include <logdepthbuf_pars_vertex>
#include <clipping_planes_pars_vertex>
varying vec2 vHighPrecisionZW;
void main() {
	#include <uv_vertex>
	#include <batching_vertex>
	#include <skinbase_vertex>
	#include <morphinstance_vertex>
	#ifdef USE_DISPLACEMENTMAP
		#include <beginnormal_vertex>
		#include <morphnormal_vertex>
		#include <skinnormal_vertex>
	#endif
	#include <begin_vertex>
	#include <morphtarget_vertex>
	#include <skinning_vertex>
	#include <displacementmap_vertex>
	#include <project_vertex>
	#include <logdepthbuf_vertex>
	#include <clipping_planes_vertex>
	vHighPrecisionZW = gl_Position.zw;
}`,Kx=`#if DEPTH_PACKING == 3200
	uniform float opacity;
#endif
#include <common>
#include <packing>
#include <uv_pars_fragment>
#include <map_pars_fragment>
#include <alphamap_pars_fragment>
#include <alphatest_pars_fragment>
#include <alphahash_pars_fragment>
#include <logdepthbuf_pars_fragment>
#include <clipping_planes_pars_fragment>
varying vec2 vHighPrecisionZW;
void main() {
	vec4 diffuseColor = vec4( 1.0 );
	#include <clipping_planes_fragment>
	#if DEPTH_PACKING == 3200
		diffuseColor.a = opacity;
	#endif
	#include <map_fragment>
	#include <alphamap_fragment>
	#include <alphatest_fragment>
	#include <alphahash_fragment>
	#include <logdepthbuf_fragment>
	#ifdef USE_REVERSED_DEPTH_BUFFER
		float fragCoordZ = vHighPrecisionZW[ 0 ] / vHighPrecisionZW[ 1 ];
	#else
		float fragCoordZ = 0.5 * vHighPrecisionZW[ 0 ] / vHighPrecisionZW[ 1 ] + 0.5;
	#endif
	#if DEPTH_PACKING == 3200
		gl_FragColor = vec4( vec3( 1.0 - fragCoordZ ), opacity );
	#elif DEPTH_PACKING == 3201
		gl_FragColor = packDepthToRGBA( fragCoordZ );
	#elif DEPTH_PACKING == 3202
		gl_FragColor = vec4( packDepthToRGB( fragCoordZ ), 1.0 );
	#elif DEPTH_PACKING == 3203
		gl_FragColor = vec4( packDepthToRG( fragCoordZ ), 0.0, 1.0 );
	#endif
}`,Jx=`#define DISTANCE
varying vec3 vWorldPosition;
#include <common>
#include <batching_pars_vertex>
#include <uv_pars_vertex>
#include <displacementmap_pars_vertex>
#include <morphtarget_pars_vertex>
#include <skinning_pars_vertex>
#include <clipping_planes_pars_vertex>
void main() {
	#include <uv_vertex>
	#include <batching_vertex>
	#include <skinbase_vertex>
	#include <morphinstance_vertex>
	#ifdef USE_DISPLACEMENTMAP
		#include <beginnormal_vertex>
		#include <morphnormal_vertex>
		#include <skinnormal_vertex>
	#endif
	#include <begin_vertex>
	#include <morphtarget_vertex>
	#include <skinning_vertex>
	#include <displacementmap_vertex>
	#include <project_vertex>
	#include <worldpos_vertex>
	#include <clipping_planes_vertex>
	vWorldPosition = worldPosition.xyz;
}`,Qx=`#define DISTANCE
uniform vec3 referencePosition;
uniform float nearDistance;
uniform float farDistance;
varying vec3 vWorldPosition;
#include <common>
#include <packing>
#include <uv_pars_fragment>
#include <map_pars_fragment>
#include <alphamap_pars_fragment>
#include <alphatest_pars_fragment>
#include <alphahash_pars_fragment>
#include <clipping_planes_pars_fragment>
void main () {
	vec4 diffuseColor = vec4( 1.0 );
	#include <clipping_planes_fragment>
	#include <map_fragment>
	#include <alphamap_fragment>
	#include <alphatest_fragment>
	#include <alphahash_fragment>
	float dist = length( vWorldPosition - referencePosition );
	dist = ( dist - nearDistance ) / ( farDistance - nearDistance );
	dist = saturate( dist );
	gl_FragColor = packDepthToRGBA( dist );
}`,eS=`varying vec3 vWorldDirection;
#include <common>
void main() {
	vWorldDirection = transformDirection( position, modelMatrix );
	#include <begin_vertex>
	#include <project_vertex>
}`,tS=`uniform sampler2D tEquirect;
varying vec3 vWorldDirection;
#include <common>
void main() {
	vec3 direction = normalize( vWorldDirection );
	vec2 sampleUV = equirectUv( direction );
	gl_FragColor = texture2D( tEquirect, sampleUV );
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
}`,nS=`uniform float scale;
attribute float lineDistance;
varying float vLineDistance;
#include <common>
#include <uv_pars_vertex>
#include <color_pars_vertex>
#include <fog_pars_vertex>
#include <morphtarget_pars_vertex>
#include <logdepthbuf_pars_vertex>
#include <clipping_planes_pars_vertex>
void main() {
	vLineDistance = scale * lineDistance;
	#include <uv_vertex>
	#include <color_vertex>
	#include <morphinstance_vertex>
	#include <morphcolor_vertex>
	#include <begin_vertex>
	#include <morphtarget_vertex>
	#include <project_vertex>
	#include <logdepthbuf_vertex>
	#include <clipping_planes_vertex>
	#include <fog_vertex>
}`,iS=`uniform vec3 diffuse;
uniform float opacity;
uniform float dashSize;
uniform float totalSize;
varying float vLineDistance;
#include <common>
#include <color_pars_fragment>
#include <uv_pars_fragment>
#include <map_pars_fragment>
#include <fog_pars_fragment>
#include <logdepthbuf_pars_fragment>
#include <clipping_planes_pars_fragment>
void main() {
	vec4 diffuseColor = vec4( diffuse, opacity );
	#include <clipping_planes_fragment>
	if ( mod( vLineDistance, totalSize ) > dashSize ) {
		discard;
	}
	vec3 outgoingLight = vec3( 0.0 );
	#include <logdepthbuf_fragment>
	#include <map_fragment>
	#include <color_fragment>
	outgoingLight = diffuseColor.rgb;
	#include <opaque_fragment>
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
	#include <fog_fragment>
	#include <premultiplied_alpha_fragment>
}`,rS=`#include <common>
#include <batching_pars_vertex>
#include <uv_pars_vertex>
#include <envmap_pars_vertex>
#include <color_pars_vertex>
#include <fog_pars_vertex>
#include <morphtarget_pars_vertex>
#include <skinning_pars_vertex>
#include <logdepthbuf_pars_vertex>
#include <clipping_planes_pars_vertex>
void main() {
	#include <uv_vertex>
	#include <color_vertex>
	#include <morphinstance_vertex>
	#include <morphcolor_vertex>
	#include <batching_vertex>
	#if defined ( USE_ENVMAP ) || defined ( USE_SKINNING )
		#include <beginnormal_vertex>
		#include <morphnormal_vertex>
		#include <skinbase_vertex>
		#include <skinnormal_vertex>
		#include <defaultnormal_vertex>
	#endif
	#include <begin_vertex>
	#include <morphtarget_vertex>
	#include <skinning_vertex>
	#include <project_vertex>
	#include <logdepthbuf_vertex>
	#include <clipping_planes_vertex>
	#include <worldpos_vertex>
	#include <envmap_vertex>
	#include <fog_vertex>
}`,sS=`uniform vec3 diffuse;
uniform float opacity;
#ifndef FLAT_SHADED
	varying vec3 vNormal;
#endif
#include <common>
#include <dithering_pars_fragment>
#include <color_pars_fragment>
#include <uv_pars_fragment>
#include <map_pars_fragment>
#include <alphamap_pars_fragment>
#include <alphatest_pars_fragment>
#include <alphahash_pars_fragment>
#include <aomap_pars_fragment>
#include <lightmap_pars_fragment>
#include <envmap_common_pars_fragment>
#include <envmap_pars_fragment>
#include <fog_pars_fragment>
#include <specularmap_pars_fragment>
#include <logdepthbuf_pars_fragment>
#include <clipping_planes_pars_fragment>
void main() {
	vec4 diffuseColor = vec4( diffuse, opacity );
	#include <clipping_planes_fragment>
	#include <logdepthbuf_fragment>
	#include <map_fragment>
	#include <color_fragment>
	#include <alphamap_fragment>
	#include <alphatest_fragment>
	#include <alphahash_fragment>
	#include <specularmap_fragment>
	ReflectedLight reflectedLight = ReflectedLight( vec3( 0.0 ), vec3( 0.0 ), vec3( 0.0 ), vec3( 0.0 ) );
	#ifdef USE_LIGHTMAP
		vec4 lightMapTexel = texture2D( lightMap, vLightMapUv );
		reflectedLight.indirectDiffuse += lightMapTexel.rgb * lightMapIntensity * RECIPROCAL_PI;
	#else
		reflectedLight.indirectDiffuse += vec3( 1.0 );
	#endif
	#include <aomap_fragment>
	reflectedLight.indirectDiffuse *= diffuseColor.rgb;
	vec3 outgoingLight = reflectedLight.indirectDiffuse;
	#include <envmap_fragment>
	#include <opaque_fragment>
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
	#include <fog_fragment>
	#include <premultiplied_alpha_fragment>
	#include <dithering_fragment>
}`,aS=`#define LAMBERT
varying vec3 vViewPosition;
#include <common>
#include <batching_pars_vertex>
#include <uv_pars_vertex>
#include <displacementmap_pars_vertex>
#include <envmap_pars_vertex>
#include <color_pars_vertex>
#include <fog_pars_vertex>
#include <normal_pars_vertex>
#include <morphtarget_pars_vertex>
#include <skinning_pars_vertex>
#include <shadowmap_pars_vertex>
#include <logdepthbuf_pars_vertex>
#include <clipping_planes_pars_vertex>
void main() {
	#include <uv_vertex>
	#include <color_vertex>
	#include <morphinstance_vertex>
	#include <morphcolor_vertex>
	#include <batching_vertex>
	#include <beginnormal_vertex>
	#include <morphnormal_vertex>
	#include <skinbase_vertex>
	#include <skinnormal_vertex>
	#include <defaultnormal_vertex>
	#include <normal_vertex>
	#include <begin_vertex>
	#include <morphtarget_vertex>
	#include <skinning_vertex>
	#include <displacementmap_vertex>
	#include <project_vertex>
	#include <logdepthbuf_vertex>
	#include <clipping_planes_vertex>
	vViewPosition = - mvPosition.xyz;
	#include <worldpos_vertex>
	#include <envmap_vertex>
	#include <shadowmap_vertex>
	#include <fog_vertex>
}`,oS=`#define LAMBERT
uniform vec3 diffuse;
uniform vec3 emissive;
uniform float opacity;
#include <common>
#include <packing>
#include <dithering_pars_fragment>
#include <color_pars_fragment>
#include <uv_pars_fragment>
#include <map_pars_fragment>
#include <alphamap_pars_fragment>
#include <alphatest_pars_fragment>
#include <alphahash_pars_fragment>
#include <aomap_pars_fragment>
#include <lightmap_pars_fragment>
#include <emissivemap_pars_fragment>
#include <envmap_common_pars_fragment>
#include <envmap_pars_fragment>
#include <fog_pars_fragment>
#include <bsdfs>
#include <lights_pars_begin>
#include <normal_pars_fragment>
#include <lights_lambert_pars_fragment>
#include <shadowmap_pars_fragment>
#include <bumpmap_pars_fragment>
#include <normalmap_pars_fragment>
#include <specularmap_pars_fragment>
#include <logdepthbuf_pars_fragment>
#include <clipping_planes_pars_fragment>
void main() {
	vec4 diffuseColor = vec4( diffuse, opacity );
	#include <clipping_planes_fragment>
	ReflectedLight reflectedLight = ReflectedLight( vec3( 0.0 ), vec3( 0.0 ), vec3( 0.0 ), vec3( 0.0 ) );
	vec3 totalEmissiveRadiance = emissive;
	#include <logdepthbuf_fragment>
	#include <map_fragment>
	#include <color_fragment>
	#include <alphamap_fragment>
	#include <alphatest_fragment>
	#include <alphahash_fragment>
	#include <specularmap_fragment>
	#include <normal_fragment_begin>
	#include <normal_fragment_maps>
	#include <emissivemap_fragment>
	#include <lights_lambert_fragment>
	#include <lights_fragment_begin>
	#include <lights_fragment_maps>
	#include <lights_fragment_end>
	#include <aomap_fragment>
	vec3 outgoingLight = reflectedLight.directDiffuse + reflectedLight.indirectDiffuse + totalEmissiveRadiance;
	#include <envmap_fragment>
	#include <opaque_fragment>
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
	#include <fog_fragment>
	#include <premultiplied_alpha_fragment>
	#include <dithering_fragment>
}`,cS=`#define MATCAP
varying vec3 vViewPosition;
#include <common>
#include <batching_pars_vertex>
#include <uv_pars_vertex>
#include <color_pars_vertex>
#include <displacementmap_pars_vertex>
#include <fog_pars_vertex>
#include <normal_pars_vertex>
#include <morphtarget_pars_vertex>
#include <skinning_pars_vertex>
#include <logdepthbuf_pars_vertex>
#include <clipping_planes_pars_vertex>
void main() {
	#include <uv_vertex>
	#include <color_vertex>
	#include <morphinstance_vertex>
	#include <morphcolor_vertex>
	#include <batching_vertex>
	#include <beginnormal_vertex>
	#include <morphnormal_vertex>
	#include <skinbase_vertex>
	#include <skinnormal_vertex>
	#include <defaultnormal_vertex>
	#include <normal_vertex>
	#include <begin_vertex>
	#include <morphtarget_vertex>
	#include <skinning_vertex>
	#include <displacementmap_vertex>
	#include <project_vertex>
	#include <logdepthbuf_vertex>
	#include <clipping_planes_vertex>
	#include <fog_vertex>
	vViewPosition = - mvPosition.xyz;
}`,lS=`#define MATCAP
uniform vec3 diffuse;
uniform float opacity;
uniform sampler2D matcap;
varying vec3 vViewPosition;
#include <common>
#include <dithering_pars_fragment>
#include <color_pars_fragment>
#include <uv_pars_fragment>
#include <map_pars_fragment>
#include <alphamap_pars_fragment>
#include <alphatest_pars_fragment>
#include <alphahash_pars_fragment>
#include <fog_pars_fragment>
#include <normal_pars_fragment>
#include <bumpmap_pars_fragment>
#include <normalmap_pars_fragment>
#include <logdepthbuf_pars_fragment>
#include <clipping_planes_pars_fragment>
void main() {
	vec4 diffuseColor = vec4( diffuse, opacity );
	#include <clipping_planes_fragment>
	#include <logdepthbuf_fragment>
	#include <map_fragment>
	#include <color_fragment>
	#include <alphamap_fragment>
	#include <alphatest_fragment>
	#include <alphahash_fragment>
	#include <normal_fragment_begin>
	#include <normal_fragment_maps>
	vec3 viewDir = normalize( vViewPosition );
	vec3 x = normalize( vec3( viewDir.z, 0.0, - viewDir.x ) );
	vec3 y = cross( viewDir, x );
	vec2 uv = vec2( dot( x, normal ), dot( y, normal ) ) * 0.495 + 0.5;
	#ifdef USE_MATCAP
		vec4 matcapColor = texture2D( matcap, uv );
	#else
		vec4 matcapColor = vec4( vec3( mix( 0.2, 0.8, uv.y ) ), 1.0 );
	#endif
	vec3 outgoingLight = diffuseColor.rgb * matcapColor.rgb;
	#include <opaque_fragment>
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
	#include <fog_fragment>
	#include <premultiplied_alpha_fragment>
	#include <dithering_fragment>
}`,uS=`#define NORMAL
#if defined( FLAT_SHADED ) || defined( USE_BUMPMAP ) || defined( USE_NORMALMAP_TANGENTSPACE )
	varying vec3 vViewPosition;
#endif
#include <common>
#include <batching_pars_vertex>
#include <uv_pars_vertex>
#include <displacementmap_pars_vertex>
#include <normal_pars_vertex>
#include <morphtarget_pars_vertex>
#include <skinning_pars_vertex>
#include <logdepthbuf_pars_vertex>
#include <clipping_planes_pars_vertex>
void main() {
	#include <uv_vertex>
	#include <batching_vertex>
	#include <beginnormal_vertex>
	#include <morphinstance_vertex>
	#include <morphnormal_vertex>
	#include <skinbase_vertex>
	#include <skinnormal_vertex>
	#include <defaultnormal_vertex>
	#include <normal_vertex>
	#include <begin_vertex>
	#include <morphtarget_vertex>
	#include <skinning_vertex>
	#include <displacementmap_vertex>
	#include <project_vertex>
	#include <logdepthbuf_vertex>
	#include <clipping_planes_vertex>
#if defined( FLAT_SHADED ) || defined( USE_BUMPMAP ) || defined( USE_NORMALMAP_TANGENTSPACE )
	vViewPosition = - mvPosition.xyz;
#endif
}`,hS=`#define NORMAL
uniform float opacity;
#if defined( FLAT_SHADED ) || defined( USE_BUMPMAP ) || defined( USE_NORMALMAP_TANGENTSPACE )
	varying vec3 vViewPosition;
#endif
#include <packing>
#include <uv_pars_fragment>
#include <normal_pars_fragment>
#include <bumpmap_pars_fragment>
#include <normalmap_pars_fragment>
#include <logdepthbuf_pars_fragment>
#include <clipping_planes_pars_fragment>
void main() {
	vec4 diffuseColor = vec4( 0.0, 0.0, 0.0, opacity );
	#include <clipping_planes_fragment>
	#include <logdepthbuf_fragment>
	#include <normal_fragment_begin>
	#include <normal_fragment_maps>
	gl_FragColor = vec4( packNormalToRGB( normal ), diffuseColor.a );
	#ifdef OPAQUE
		gl_FragColor.a = 1.0;
	#endif
}`,dS=`#define PHONG
varying vec3 vViewPosition;
#include <common>
#include <batching_pars_vertex>
#include <uv_pars_vertex>
#include <displacementmap_pars_vertex>
#include <envmap_pars_vertex>
#include <color_pars_vertex>
#include <fog_pars_vertex>
#include <normal_pars_vertex>
#include <morphtarget_pars_vertex>
#include <skinning_pars_vertex>
#include <shadowmap_pars_vertex>
#include <logdepthbuf_pars_vertex>
#include <clipping_planes_pars_vertex>
void main() {
	#include <uv_vertex>
	#include <color_vertex>
	#include <morphcolor_vertex>
	#include <batching_vertex>
	#include <beginnormal_vertex>
	#include <morphinstance_vertex>
	#include <morphnormal_vertex>
	#include <skinbase_vertex>
	#include <skinnormal_vertex>
	#include <defaultnormal_vertex>
	#include <normal_vertex>
	#include <begin_vertex>
	#include <morphtarget_vertex>
	#include <skinning_vertex>
	#include <displacementmap_vertex>
	#include <project_vertex>
	#include <logdepthbuf_vertex>
	#include <clipping_planes_vertex>
	vViewPosition = - mvPosition.xyz;
	#include <worldpos_vertex>
	#include <envmap_vertex>
	#include <shadowmap_vertex>
	#include <fog_vertex>
}`,fS=`#define PHONG
uniform vec3 diffuse;
uniform vec3 emissive;
uniform vec3 specular;
uniform float shininess;
uniform float opacity;
#include <common>
#include <packing>
#include <dithering_pars_fragment>
#include <color_pars_fragment>
#include <uv_pars_fragment>
#include <map_pars_fragment>
#include <alphamap_pars_fragment>
#include <alphatest_pars_fragment>
#include <alphahash_pars_fragment>
#include <aomap_pars_fragment>
#include <lightmap_pars_fragment>
#include <emissivemap_pars_fragment>
#include <envmap_common_pars_fragment>
#include <envmap_pars_fragment>
#include <fog_pars_fragment>
#include <bsdfs>
#include <lights_pars_begin>
#include <normal_pars_fragment>
#include <lights_phong_pars_fragment>
#include <shadowmap_pars_fragment>
#include <bumpmap_pars_fragment>
#include <normalmap_pars_fragment>
#include <specularmap_pars_fragment>
#include <logdepthbuf_pars_fragment>
#include <clipping_planes_pars_fragment>
void main() {
	vec4 diffuseColor = vec4( diffuse, opacity );
	#include <clipping_planes_fragment>
	ReflectedLight reflectedLight = ReflectedLight( vec3( 0.0 ), vec3( 0.0 ), vec3( 0.0 ), vec3( 0.0 ) );
	vec3 totalEmissiveRadiance = emissive;
	#include <logdepthbuf_fragment>
	#include <map_fragment>
	#include <color_fragment>
	#include <alphamap_fragment>
	#include <alphatest_fragment>
	#include <alphahash_fragment>
	#include <specularmap_fragment>
	#include <normal_fragment_begin>
	#include <normal_fragment_maps>
	#include <emissivemap_fragment>
	#include <lights_phong_fragment>
	#include <lights_fragment_begin>
	#include <lights_fragment_maps>
	#include <lights_fragment_end>
	#include <aomap_fragment>
	vec3 outgoingLight = reflectedLight.directDiffuse + reflectedLight.indirectDiffuse + reflectedLight.directSpecular + reflectedLight.indirectSpecular + totalEmissiveRadiance;
	#include <envmap_fragment>
	#include <opaque_fragment>
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
	#include <fog_fragment>
	#include <premultiplied_alpha_fragment>
	#include <dithering_fragment>
}`,pS=`#define STANDARD
varying vec3 vViewPosition;
#ifdef USE_TRANSMISSION
	varying vec3 vWorldPosition;
#endif
#include <common>
#include <batching_pars_vertex>
#include <uv_pars_vertex>
#include <displacementmap_pars_vertex>
#include <color_pars_vertex>
#include <fog_pars_vertex>
#include <normal_pars_vertex>
#include <morphtarget_pars_vertex>
#include <skinning_pars_vertex>
#include <shadowmap_pars_vertex>
#include <logdepthbuf_pars_vertex>
#include <clipping_planes_pars_vertex>
void main() {
	#include <uv_vertex>
	#include <color_vertex>
	#include <morphinstance_vertex>
	#include <morphcolor_vertex>
	#include <batching_vertex>
	#include <beginnormal_vertex>
	#include <morphnormal_vertex>
	#include <skinbase_vertex>
	#include <skinnormal_vertex>
	#include <defaultnormal_vertex>
	#include <normal_vertex>
	#include <begin_vertex>
	#include <morphtarget_vertex>
	#include <skinning_vertex>
	#include <displacementmap_vertex>
	#include <project_vertex>
	#include <logdepthbuf_vertex>
	#include <clipping_planes_vertex>
	vViewPosition = - mvPosition.xyz;
	#include <worldpos_vertex>
	#include <shadowmap_vertex>
	#include <fog_vertex>
#ifdef USE_TRANSMISSION
	vWorldPosition = worldPosition.xyz;
#endif
}`,mS=`#define STANDARD
#ifdef PHYSICAL
	#define IOR
	#define USE_SPECULAR
#endif
uniform vec3 diffuse;
uniform vec3 emissive;
uniform float roughness;
uniform float metalness;
uniform float opacity;
#ifdef IOR
	uniform float ior;
#endif
#ifdef USE_SPECULAR
	uniform float specularIntensity;
	uniform vec3 specularColor;
	#ifdef USE_SPECULAR_COLORMAP
		uniform sampler2D specularColorMap;
	#endif
	#ifdef USE_SPECULAR_INTENSITYMAP
		uniform sampler2D specularIntensityMap;
	#endif
#endif
#ifdef USE_CLEARCOAT
	uniform float clearcoat;
	uniform float clearcoatRoughness;
#endif
#ifdef USE_DISPERSION
	uniform float dispersion;
#endif
#ifdef USE_IRIDESCENCE
	uniform float iridescence;
	uniform float iridescenceIOR;
	uniform float iridescenceThicknessMinimum;
	uniform float iridescenceThicknessMaximum;
#endif
#ifdef USE_SHEEN
	uniform vec3 sheenColor;
	uniform float sheenRoughness;
	#ifdef USE_SHEEN_COLORMAP
		uniform sampler2D sheenColorMap;
	#endif
	#ifdef USE_SHEEN_ROUGHNESSMAP
		uniform sampler2D sheenRoughnessMap;
	#endif
#endif
#ifdef USE_ANISOTROPY
	uniform vec2 anisotropyVector;
	#ifdef USE_ANISOTROPYMAP
		uniform sampler2D anisotropyMap;
	#endif
#endif
varying vec3 vViewPosition;
#include <common>
#include <packing>
#include <dithering_pars_fragment>
#include <color_pars_fragment>
#include <uv_pars_fragment>
#include <map_pars_fragment>
#include <alphamap_pars_fragment>
#include <alphatest_pars_fragment>
#include <alphahash_pars_fragment>
#include <aomap_pars_fragment>
#include <lightmap_pars_fragment>
#include <emissivemap_pars_fragment>
#include <iridescence_fragment>
#include <cube_uv_reflection_fragment>
#include <envmap_common_pars_fragment>
#include <envmap_physical_pars_fragment>
#include <fog_pars_fragment>
#include <lights_pars_begin>
#include <normal_pars_fragment>
#include <lights_physical_pars_fragment>
#include <transmission_pars_fragment>
#include <shadowmap_pars_fragment>
#include <bumpmap_pars_fragment>
#include <normalmap_pars_fragment>
#include <clearcoat_pars_fragment>
#include <iridescence_pars_fragment>
#include <roughnessmap_pars_fragment>
#include <metalnessmap_pars_fragment>
#include <logdepthbuf_pars_fragment>
#include <clipping_planes_pars_fragment>
void main() {
	vec4 diffuseColor = vec4( diffuse, opacity );
	#include <clipping_planes_fragment>
	ReflectedLight reflectedLight = ReflectedLight( vec3( 0.0 ), vec3( 0.0 ), vec3( 0.0 ), vec3( 0.0 ) );
	vec3 totalEmissiveRadiance = emissive;
	#include <logdepthbuf_fragment>
	#include <map_fragment>
	#include <color_fragment>
	#include <alphamap_fragment>
	#include <alphatest_fragment>
	#include <alphahash_fragment>
	#include <roughnessmap_fragment>
	#include <metalnessmap_fragment>
	#include <normal_fragment_begin>
	#include <normal_fragment_maps>
	#include <clearcoat_normal_fragment_begin>
	#include <clearcoat_normal_fragment_maps>
	#include <emissivemap_fragment>
	#include <lights_physical_fragment>
	#include <lights_fragment_begin>
	#include <lights_fragment_maps>
	#include <lights_fragment_end>
	#include <aomap_fragment>
	vec3 totalDiffuse = reflectedLight.directDiffuse + reflectedLight.indirectDiffuse;
	vec3 totalSpecular = reflectedLight.directSpecular + reflectedLight.indirectSpecular;
	#include <transmission_fragment>
	vec3 outgoingLight = totalDiffuse + totalSpecular + totalEmissiveRadiance;
	#ifdef USE_SHEEN
		float sheenEnergyComp = 1.0 - 0.157 * max3( material.sheenColor );
		outgoingLight = outgoingLight * sheenEnergyComp + sheenSpecularDirect + sheenSpecularIndirect;
	#endif
	#ifdef USE_CLEARCOAT
		float dotNVcc = saturate( dot( geometryClearcoatNormal, geometryViewDir ) );
		vec3 Fcc = F_Schlick( material.clearcoatF0, material.clearcoatF90, dotNVcc );
		outgoingLight = outgoingLight * ( 1.0 - material.clearcoat * Fcc ) + ( clearcoatSpecularDirect + clearcoatSpecularIndirect ) * material.clearcoat;
	#endif
	#include <opaque_fragment>
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
	#include <fog_fragment>
	#include <premultiplied_alpha_fragment>
	#include <dithering_fragment>
}`,gS=`#define TOON
varying vec3 vViewPosition;
#include <common>
#include <batching_pars_vertex>
#include <uv_pars_vertex>
#include <displacementmap_pars_vertex>
#include <color_pars_vertex>
#include <fog_pars_vertex>
#include <normal_pars_vertex>
#include <morphtarget_pars_vertex>
#include <skinning_pars_vertex>
#include <shadowmap_pars_vertex>
#include <logdepthbuf_pars_vertex>
#include <clipping_planes_pars_vertex>
void main() {
	#include <uv_vertex>
	#include <color_vertex>
	#include <morphinstance_vertex>
	#include <morphcolor_vertex>
	#include <batching_vertex>
	#include <beginnormal_vertex>
	#include <morphnormal_vertex>
	#include <skinbase_vertex>
	#include <skinnormal_vertex>
	#include <defaultnormal_vertex>
	#include <normal_vertex>
	#include <begin_vertex>
	#include <morphtarget_vertex>
	#include <skinning_vertex>
	#include <displacementmap_vertex>
	#include <project_vertex>
	#include <logdepthbuf_vertex>
	#include <clipping_planes_vertex>
	vViewPosition = - mvPosition.xyz;
	#include <worldpos_vertex>
	#include <shadowmap_vertex>
	#include <fog_vertex>
}`,_S=`#define TOON
uniform vec3 diffuse;
uniform vec3 emissive;
uniform float opacity;
#include <common>
#include <packing>
#include <dithering_pars_fragment>
#include <color_pars_fragment>
#include <uv_pars_fragment>
#include <map_pars_fragment>
#include <alphamap_pars_fragment>
#include <alphatest_pars_fragment>
#include <alphahash_pars_fragment>
#include <aomap_pars_fragment>
#include <lightmap_pars_fragment>
#include <emissivemap_pars_fragment>
#include <gradientmap_pars_fragment>
#include <fog_pars_fragment>
#include <bsdfs>
#include <lights_pars_begin>
#include <normal_pars_fragment>
#include <lights_toon_pars_fragment>
#include <shadowmap_pars_fragment>
#include <bumpmap_pars_fragment>
#include <normalmap_pars_fragment>
#include <logdepthbuf_pars_fragment>
#include <clipping_planes_pars_fragment>
void main() {
	vec4 diffuseColor = vec4( diffuse, opacity );
	#include <clipping_planes_fragment>
	ReflectedLight reflectedLight = ReflectedLight( vec3( 0.0 ), vec3( 0.0 ), vec3( 0.0 ), vec3( 0.0 ) );
	vec3 totalEmissiveRadiance = emissive;
	#include <logdepthbuf_fragment>
	#include <map_fragment>
	#include <color_fragment>
	#include <alphamap_fragment>
	#include <alphatest_fragment>
	#include <alphahash_fragment>
	#include <normal_fragment_begin>
	#include <normal_fragment_maps>
	#include <emissivemap_fragment>
	#include <lights_toon_fragment>
	#include <lights_fragment_begin>
	#include <lights_fragment_maps>
	#include <lights_fragment_end>
	#include <aomap_fragment>
	vec3 outgoingLight = reflectedLight.directDiffuse + reflectedLight.indirectDiffuse + totalEmissiveRadiance;
	#include <opaque_fragment>
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
	#include <fog_fragment>
	#include <premultiplied_alpha_fragment>
	#include <dithering_fragment>
}`,yS=`uniform float size;
uniform float scale;
#include <common>
#include <color_pars_vertex>
#include <fog_pars_vertex>
#include <morphtarget_pars_vertex>
#include <logdepthbuf_pars_vertex>
#include <clipping_planes_pars_vertex>
#ifdef USE_POINTS_UV
	varying vec2 vUv;
	uniform mat3 uvTransform;
#endif
void main() {
	#ifdef USE_POINTS_UV
		vUv = ( uvTransform * vec3( uv, 1 ) ).xy;
	#endif
	#include <color_vertex>
	#include <morphinstance_vertex>
	#include <morphcolor_vertex>
	#include <begin_vertex>
	#include <morphtarget_vertex>
	#include <project_vertex>
	gl_PointSize = size;
	#ifdef USE_SIZEATTENUATION
		bool isPerspective = isPerspectiveMatrix( projectionMatrix );
		if ( isPerspective ) gl_PointSize *= ( scale / - mvPosition.z );
	#endif
	#include <logdepthbuf_vertex>
	#include <clipping_planes_vertex>
	#include <worldpos_vertex>
	#include <fog_vertex>
}`,vS=`uniform vec3 diffuse;
uniform float opacity;
#include <common>
#include <color_pars_fragment>
#include <map_particle_pars_fragment>
#include <alphatest_pars_fragment>
#include <alphahash_pars_fragment>
#include <fog_pars_fragment>
#include <logdepthbuf_pars_fragment>
#include <clipping_planes_pars_fragment>
void main() {
	vec4 diffuseColor = vec4( diffuse, opacity );
	#include <clipping_planes_fragment>
	vec3 outgoingLight = vec3( 0.0 );
	#include <logdepthbuf_fragment>
	#include <map_particle_fragment>
	#include <color_fragment>
	#include <alphatest_fragment>
	#include <alphahash_fragment>
	outgoingLight = diffuseColor.rgb;
	#include <opaque_fragment>
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
	#include <fog_fragment>
	#include <premultiplied_alpha_fragment>
}`,xS=`#include <common>
#include <batching_pars_vertex>
#include <fog_pars_vertex>
#include <morphtarget_pars_vertex>
#include <skinning_pars_vertex>
#include <logdepthbuf_pars_vertex>
#include <shadowmap_pars_vertex>
void main() {
	#include <batching_vertex>
	#include <beginnormal_vertex>
	#include <morphinstance_vertex>
	#include <morphnormal_vertex>
	#include <skinbase_vertex>
	#include <skinnormal_vertex>
	#include <defaultnormal_vertex>
	#include <begin_vertex>
	#include <morphtarget_vertex>
	#include <skinning_vertex>
	#include <project_vertex>
	#include <logdepthbuf_vertex>
	#include <worldpos_vertex>
	#include <shadowmap_vertex>
	#include <fog_vertex>
}`,SS=`uniform vec3 color;
uniform float opacity;
#include <common>
#include <packing>
#include <fog_pars_fragment>
#include <bsdfs>
#include <lights_pars_begin>
#include <logdepthbuf_pars_fragment>
#include <shadowmap_pars_fragment>
#include <shadowmask_pars_fragment>
void main() {
	#include <logdepthbuf_fragment>
	gl_FragColor = vec4( color, opacity * ( 1.0 - getShadowMask() ) );
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
	#include <fog_fragment>
}`,bS=`uniform float rotation;
uniform vec2 center;
#include <common>
#include <uv_pars_vertex>
#include <fog_pars_vertex>
#include <logdepthbuf_pars_vertex>
#include <clipping_planes_pars_vertex>
void main() {
	#include <uv_vertex>
	vec4 mvPosition = modelViewMatrix[ 3 ];
	vec2 scale = vec2( length( modelMatrix[ 0 ].xyz ), length( modelMatrix[ 1 ].xyz ) );
	#ifndef USE_SIZEATTENUATION
		bool isPerspective = isPerspectiveMatrix( projectionMatrix );
		if ( isPerspective ) scale *= - mvPosition.z;
	#endif
	vec2 alignedPosition = ( position.xy - ( center - vec2( 0.5 ) ) ) * scale;
	vec2 rotatedPosition;
	rotatedPosition.x = cos( rotation ) * alignedPosition.x - sin( rotation ) * alignedPosition.y;
	rotatedPosition.y = sin( rotation ) * alignedPosition.x + cos( rotation ) * alignedPosition.y;
	mvPosition.xy += rotatedPosition;
	gl_Position = projectionMatrix * mvPosition;
	#include <logdepthbuf_vertex>
	#include <clipping_planes_vertex>
	#include <fog_vertex>
}`,MS=`uniform vec3 diffuse;
uniform float opacity;
#include <common>
#include <uv_pars_fragment>
#include <map_pars_fragment>
#include <alphamap_pars_fragment>
#include <alphatest_pars_fragment>
#include <alphahash_pars_fragment>
#include <fog_pars_fragment>
#include <logdepthbuf_pars_fragment>
#include <clipping_planes_pars_fragment>
void main() {
	vec4 diffuseColor = vec4( diffuse, opacity );
	#include <clipping_planes_fragment>
	vec3 outgoingLight = vec3( 0.0 );
	#include <logdepthbuf_fragment>
	#include <map_fragment>
	#include <alphamap_fragment>
	#include <alphatest_fragment>
	#include <alphahash_fragment>
	outgoingLight = diffuseColor.rgb;
	#include <opaque_fragment>
	#include <tonemapping_fragment>
	#include <colorspace_fragment>
	#include <fog_fragment>
}`,He={alphahash_fragment:Wy,alphahash_pars_fragment:qy,alphamap_fragment:Xy,alphamap_pars_fragment:jy,alphatest_fragment:Yy,alphatest_pars_fragment:Zy,aomap_fragment:Ky,aomap_pars_fragment:Jy,batching_pars_vertex:Qy,batching_vertex:ev,begin_vertex:tv,beginnormal_vertex:nv,bsdfs:iv,iridescence_fragment:rv,bumpmap_pars_fragment:sv,clipping_planes_fragment:av,clipping_planes_pars_fragment:ov,clipping_planes_pars_vertex:cv,clipping_planes_vertex:lv,color_fragment:uv,color_pars_fragment:hv,color_pars_vertex:dv,color_vertex:fv,common:pv,cube_uv_reflection_fragment:mv,defaultnormal_vertex:gv,displacementmap_pars_vertex:_v,displacementmap_vertex:yv,emissivemap_fragment:vv,emissivemap_pars_fragment:xv,colorspace_fragment:Sv,colorspace_pars_fragment:bv,envmap_fragment:Mv,envmap_common_pars_fragment:Ev,envmap_pars_fragment:wv,envmap_pars_vertex:Tv,envmap_physical_pars_fragment:Fv,envmap_vertex:Av,fog_vertex:Rv,fog_pars_vertex:Cv,fog_fragment:Pv,fog_pars_fragment:Iv,gradientmap_pars_fragment:Dv,lightmap_pars_fragment:Nv,lights_lambert_fragment:Lv,lights_lambert_pars_fragment:Uv,lights_pars_begin:Ov,lights_toon_fragment:Bv,lights_toon_pars_fragment:kv,lights_phong_fragment:zv,lights_phong_pars_fragment:Vv,lights_physical_fragment:$v,lights_physical_pars_fragment:Hv,lights_fragment_begin:Gv,lights_fragment_maps:Wv,lights_fragment_end:qv,logdepthbuf_fragment:Xv,logdepthbuf_pars_fragment:jv,logdepthbuf_pars_vertex:Yv,logdepthbuf_vertex:Zv,map_fragment:Kv,map_pars_fragment:Jv,map_particle_fragment:Qv,map_particle_pars_fragment:ex,metalnessmap_fragment:tx,metalnessmap_pars_fragment:nx,morphinstance_vertex:ix,morphcolor_vertex:rx,morphnormal_vertex:sx,morphtarget_pars_vertex:ax,morphtarget_vertex:ox,normal_fragment_begin:cx,normal_fragment_maps:lx,normal_pars_fragment:ux,normal_pars_vertex:hx,normal_vertex:dx,normalmap_pars_fragment:fx,clearcoat_normal_fragment_begin:px,clearcoat_normal_fragment_maps:mx,clearcoat_pars_fragment:gx,iridescence_pars_fragment:_x,opaque_fragment:yx,packing:vx,premultiplied_alpha_fragment:xx,project_vertex:Sx,dithering_fragment:bx,dithering_pars_fragment:Mx,roughnessmap_fragment:Ex,roughnessmap_pars_fragment:wx,shadowmap_pars_fragment:Tx,shadowmap_pars_vertex:Ax,shadowmap_vertex:Rx,shadowmask_pars_fragment:Cx,skinbase_vertex:Px,skinning_pars_vertex:Ix,skinning_vertex:Dx,skinnormal_vertex:Nx,specularmap_fragment:Lx,specularmap_pars_fragment:Ux,tonemapping_fragment:Ox,tonemapping_pars_fragment:Fx,transmission_fragment:Bx,transmission_pars_fragment:kx,uv_pars_fragment:zx,uv_pars_vertex:Vx,uv_vertex:$x,worldpos_vertex:Hx,background_vert:Gx,background_frag:Wx,backgroundCube_vert:qx,backgroundCube_frag:Xx,cube_vert:jx,cube_frag:Yx,depth_vert:Zx,depth_frag:Kx,distanceRGBA_vert:Jx,distanceRGBA_frag:Qx,equirect_vert:eS,equirect_frag:tS,linedashed_vert:nS,linedashed_frag:iS,meshbasic_vert:rS,meshbasic_frag:sS,meshlambert_vert:aS,meshlambert_frag:oS,meshmatcap_vert:cS,meshmatcap_frag:lS,meshnormal_vert:uS,meshnormal_frag:hS,meshphong_vert:dS,meshphong_frag:fS,meshphysical_vert:pS,meshphysical_frag:mS,meshtoon_vert:gS,meshtoon_frag:_S,points_vert:yS,points_frag:vS,shadow_vert:xS,shadow_frag:SS,sprite_vert:bS,sprite_frag:MS},se={common:{diffuse:{value:new Qe(16777215)},opacity:{value:1},map:{value:null},mapTransform:{value:new Fe},alphaMap:{value:null},alphaMapTransform:{value:new Fe},alphaTest:{value:0}},specularmap:{specularMap:{value:null},specularMapTransform:{value:new Fe}},envmap:{envMap:{value:null},envMapRotation:{value:new Fe},flipEnvMap:{value:-1},reflectivity:{value:1},ior:{value:1.5},refractionRatio:{value:.98}},aomap:{aoMap:{value:null},aoMapIntensity:{value:1},aoMapTransform:{value:new Fe}},lightmap:{lightMap:{value:null},lightMapIntensity:{value:1},lightMapTransform:{value:new Fe}},bumpmap:{bumpMap:{value:null},bumpMapTransform:{value:new Fe},bumpScale:{value:1}},normalmap:{normalMap:{value:null},normalMapTransform:{value:new Fe},normalScale:{value:new Ue(1,1)}},displacementmap:{displacementMap:{value:null},displacementMapTransform:{value:new Fe},displacementScale:{value:1},displacementBias:{value:0}},emissivemap:{emissiveMap:{value:null},emissiveMapTransform:{value:new Fe}},metalnessmap:{metalnessMap:{value:null},metalnessMapTransform:{value:new Fe}},roughnessmap:{roughnessMap:{value:null},roughnessMapTransform:{value:new Fe}},gradientmap:{gradientMap:{value:null}},fog:{fogDensity:{value:25e-5},fogNear:{value:1},fogFar:{value:2e3},fogColor:{value:new Qe(16777215)}},lights:{ambientLightColor:{value:[]},lightProbe:{value:[]},directionalLights:{value:[],properties:{direction:{},color:{}}},directionalLightShadows:{value:[],properties:{shadowIntensity:1,shadowBias:{},shadowNormalBias:{},shadowRadius:{},shadowMapSize:{}}},directionalShadowMap:{value:[]},directionalShadowMatrix:{value:[]},spotLights:{value:[],properties:{color:{},position:{},direction:{},distance:{},coneCos:{},penumbraCos:{},decay:{}}},spotLightShadows:{value:[],properties:{shadowIntensity:1,shadowBias:{},shadowNormalBias:{},shadowRadius:{},shadowMapSize:{}}},spotLightMap:{value:[]},spotShadowMap:{value:[]},spotLightMatrix:{value:[]},pointLights:{value:[],properties:{color:{},position:{},decay:{},distance:{}}},pointLightShadows:{value:[],properties:{shadowIntensity:1,shadowBias:{},shadowNormalBias:{},shadowRadius:{},shadowMapSize:{},shadowCameraNear:{},shadowCameraFar:{}}},pointShadowMap:{value:[]},pointShadowMatrix:{value:[]},hemisphereLights:{value:[],properties:{direction:{},skyColor:{},groundColor:{}}},rectAreaLights:{value:[],properties:{color:{},position:{},width:{},height:{}}},ltc_1:{value:null},ltc_2:{value:null}},points:{diffuse:{value:new Qe(16777215)},opacity:{value:1},size:{value:1},scale:{value:1},map:{value:null},alphaMap:{value:null},alphaMapTransform:{value:new Fe},alphaTest:{value:0},uvTransform:{value:new Fe}},sprite:{diffuse:{value:new Qe(16777215)},opacity:{value:1},center:{value:new Ue(.5,.5)},rotation:{value:0},map:{value:null},mapTransform:{value:new Fe},alphaMap:{value:null},alphaMapTransform:{value:new Fe},alphaTest:{value:0}}},Kn={basic:{uniforms:Gt([se.common,se.specularmap,se.envmap,se.aomap,se.lightmap,se.fog]),vertexShader:He.meshbasic_vert,fragmentShader:He.meshbasic_frag},lambert:{uniforms:Gt([se.common,se.specularmap,se.envmap,se.aomap,se.lightmap,se.emissivemap,se.bumpmap,se.normalmap,se.displacementmap,se.fog,se.lights,{emissive:{value:new Qe(0)}}]),vertexShader:He.meshlambert_vert,fragmentShader:He.meshlambert_frag},phong:{uniforms:Gt([se.common,se.specularmap,se.envmap,se.aomap,se.lightmap,se.emissivemap,se.bumpmap,se.normalmap,se.displacementmap,se.fog,se.lights,{emissive:{value:new Qe(0)},specular:{value:new Qe(1118481)},shininess:{value:30}}]),vertexShader:He.meshphong_vert,fragmentShader:He.meshphong_frag},standard:{uniforms:Gt([se.common,se.envmap,se.aomap,se.lightmap,se.emissivemap,se.bumpmap,se.normalmap,se.displacementmap,se.roughnessmap,se.metalnessmap,se.fog,se.lights,{emissive:{value:new Qe(0)},roughness:{value:1},metalness:{value:0},envMapIntensity:{value:1}}]),vertexShader:He.meshphysical_vert,fragmentShader:He.meshphysical_frag},toon:{uniforms:Gt([se.common,se.aomap,se.lightmap,se.emissivemap,se.bumpmap,se.normalmap,se.displacementmap,se.gradientmap,se.fog,se.lights,{emissive:{value:new Qe(0)}}]),vertexShader:He.meshtoon_vert,fragmentShader:He.meshtoon_frag},matcap:{uniforms:Gt([se.common,se.bumpmap,se.normalmap,se.displacementmap,se.fog,{matcap:{value:null}}]),vertexShader:He.meshmatcap_vert,fragmentShader:He.meshmatcap_frag},points:{uniforms:Gt([se.points,se.fog]),vertexShader:He.points_vert,fragmentShader:He.points_frag},dashed:{uniforms:Gt([se.common,se.fog,{scale:{value:1},dashSize:{value:1},totalSize:{value:2}}]),vertexShader:He.linedashed_vert,fragmentShader:He.linedashed_frag},depth:{uniforms:Gt([se.common,se.displacementmap]),vertexShader:He.depth_vert,fragmentShader:He.depth_frag},normal:{uniforms:Gt([se.common,se.bumpmap,se.normalmap,se.displacementmap,{opacity:{value:1}}]),vertexShader:He.meshnormal_vert,fragmentShader:He.meshnormal_frag},sprite:{uniforms:Gt([se.sprite,se.fog]),vertexShader:He.sprite_vert,fragmentShader:He.sprite_frag},background:{uniforms:{uvTransform:{value:new Fe},t2D:{value:null},backgroundIntensity:{value:1}},vertexShader:He.background_vert,fragmentShader:He.background_frag},backgroundCube:{uniforms:{envMap:{value:null},flipEnvMap:{value:-1},backgroundBlurriness:{value:0},backgroundIntensity:{value:1},backgroundRotation:{value:new Fe}},vertexShader:He.backgroundCube_vert,fragmentShader:He.backgroundCube_frag},cube:{uniforms:{tCube:{value:null},tFlip:{value:-1},opacity:{value:1}},vertexShader:He.cube_vert,fragmentShader:He.cube_frag},equirect:{uniforms:{tEquirect:{value:null}},vertexShader:He.equirect_vert,fragmentShader:He.equirect_frag},distanceRGBA:{uniforms:Gt([se.common,se.displacementmap,{referencePosition:{value:new L},nearDistance:{value:1},farDistance:{value:1e3}}]),vertexShader:He.distanceRGBA_vert,fragmentShader:He.distanceRGBA_frag},shadow:{uniforms:Gt([se.lights,se.fog,{color:{value:new Qe(0)},opacity:{value:1}}]),vertexShader:He.shadow_vert,fragmentShader:He.shadow_frag}};Kn.physical={uniforms:Gt([Kn.standard.uniforms,{clearcoat:{value:0},clearcoatMap:{value:null},clearcoatMapTransform:{value:new Fe},clearcoatNormalMap:{value:null},clearcoatNormalMapTransform:{value:new Fe},clearcoatNormalScale:{value:new Ue(1,1)},clearcoatRoughness:{value:0},clearcoatRoughnessMap:{value:null},clearcoatRoughnessMapTransform:{value:new Fe},dispersion:{value:0},iridescence:{value:0},iridescenceMap:{value:null},iridescenceMapTransform:{value:new Fe},iridescenceIOR:{value:1.3},iridescenceThicknessMinimum:{value:100},iridescenceThicknessMaximum:{value:400},iridescenceThicknessMap:{value:null},iridescenceThicknessMapTransform:{value:new Fe},sheen:{value:0},sheenColor:{value:new Qe(0)},sheenColorMap:{value:null},sheenColorMapTransform:{value:new Fe},sheenRoughness:{value:1},sheenRoughnessMap:{value:null},sheenRoughnessMapTransform:{value:new Fe},transmission:{value:0},transmissionMap:{value:null},transmissionMapTransform:{value:new Fe},transmissionSamplerSize:{value:new Ue},transmissionSamplerMap:{value:null},thickness:{value:0},thicknessMap:{value:null},thicknessMapTransform:{value:new Fe},attenuationDistance:{value:0},attenuationColor:{value:new Qe(0)},specularColor:{value:new Qe(1,1,1)},specularColorMap:{value:null},specularColorMapTransform:{value:new Fe},specularIntensity:{value:1},specularIntensityMap:{value:null},specularIntensityMapTransform:{value:new Fe},anisotropyVector:{value:new Ue},anisotropyMap:{value:null},anisotropyMapTransform:{value:new Fe}}]),vertexShader:He.meshphysical_vert,fragmentShader:He.meshphysical_frag};var wc={r:0,b:0,g:0},ar=new Xn,ES=new _t;function wS(n,e,t,i,r,s,a){let o=new Qe(0),c=s===!0?0:1,u,l,h=null,f=0,p=null;function g(M){let x=M.isScene===!0?M.background:null;return x&&x.isTexture&&(x=(M.backgroundBlurriness>0?t:e).get(x)),x}function _(M){let x=!1,T=g(M);T===null?d(o,c):T&&T.isColor&&(d(T,1),x=!0);let R=n.xr.getEnvironmentBlendMode();R==="additive"?i.buffers.color.setClear(0,0,0,1,a):R==="alpha-blend"&&i.buffers.color.setClear(0,0,0,0,a),(n.autoClear||x)&&(i.buffers.depth.setTest(!0),i.buffers.depth.setMask(!0),i.buffers.color.setMask(!0),n.clear(n.autoClearColor,n.autoClearDepth,n.autoClearStencil))}function m(M,x){let T=g(x);T&&(T.isCubeTexture||T.mapping===Qs)?(l===void 0&&(l=new Ht(new Yr(1,1,1),new Fn({name:"BackgroundCubeMaterial",uniforms:sr(Kn.backgroundCube.uniforms),vertexShader:Kn.backgroundCube.vertexShader,fragmentShader:Kn.backgroundCube.fragmentShader,side:Jt,depthTest:!1,depthWrite:!1,fog:!1,allowOverride:!1})),l.geometry.deleteAttribute("normal"),l.geometry.deleteAttribute("uv"),l.onBeforeRender=function(R,P,O){this.matrixWorld.copyPosition(O.matrixWorld)},Object.defineProperty(l.material,"envMap",{get:function(){return this.uniforms.envMap.value}}),r.update(l)),ar.copy(x.backgroundRotation),ar.x*=-1,ar.y*=-1,ar.z*=-1,T.isCubeTexture&&T.isRenderTargetTexture===!1&&(ar.y*=-1,ar.z*=-1),l.material.uniforms.envMap.value=T,l.material.uniforms.flipEnvMap.value=T.isCubeTexture&&T.isRenderTargetTexture===!1?-1:1,l.material.uniforms.backgroundBlurriness.value=x.backgroundBlurriness,l.material.uniforms.backgroundIntensity.value=x.backgroundIntensity,l.material.uniforms.backgroundRotation.value.setFromMatrix4(ES.makeRotationFromEuler(ar)),l.material.toneMapped=tt.getTransfer(T.colorSpace)!==ot,(h!==T||f!==T.version||p!==n.toneMapping)&&(l.material.needsUpdate=!0,h=T,f=T.version,p=n.toneMapping),l.layers.enableAll(),M.unshift(l,l.geometry,l.material,0,0,null)):T&&T.isTexture&&(u===void 0&&(u=new Ht(new js(2,2),new Fn({name:"BackgroundMaterial",uniforms:sr(Kn.background.uniforms),vertexShader:Kn.background.vertexShader,fragmentShader:Kn.background.fragmentShader,side:ci,depthTest:!1,depthWrite:!1,fog:!1,allowOverride:!1})),u.geometry.deleteAttribute("normal"),Object.defineProperty(u.material,"map",{get:function(){return this.uniforms.t2D.value}}),r.update(u)),u.material.uniforms.t2D.value=T,u.material.uniforms.backgroundIntensity.value=x.backgroundIntensity,u.material.toneMapped=tt.getTransfer(T.colorSpace)!==ot,T.matrixAutoUpdate===!0&&T.updateMatrix(),u.material.uniforms.uvTransform.value.copy(T.matrix),(h!==T||f!==T.version||p!==n.toneMapping)&&(u.material.needsUpdate=!0,h=T,f=T.version,p=n.toneMapping),u.layers.enableAll(),M.unshift(u,u.geometry,u.material,0,0,null))}function d(M,x){M.getRGB(wc,yu(n)),i.buffers.color.setClear(wc.r,wc.g,wc.b,x,a)}function w(){l!==void 0&&(l.geometry.dispose(),l.material.dispose(),l=void 0),u!==void 0&&(u.geometry.dispose(),u.material.dispose(),u=void 0)}return{getClearColor:function(){return o},setClearColor:function(M,x=1){o.set(M),c=x,d(o,c)},getClearAlpha:function(){return c},setClearAlpha:function(M){c=M,d(o,c)},render:_,addToRenderList:m,dispose:w}}function TS(n,e){let t=n.getParameter(n.MAX_VERTEX_ATTRIBS),i={},r=f(null),s=r,a=!1;function o(S,I,k,V,W){let j=!1,G=h(V,k,I);s!==G&&(s=G,u(s.object)),j=p(S,V,k,W),j&&g(S,V,k,W),W!==null&&e.update(W,n.ELEMENT_ARRAY_BUFFER),(j||a)&&(a=!1,x(S,I,k,V),W!==null&&n.bindBuffer(n.ELEMENT_ARRAY_BUFFER,e.get(W).buffer))}function c(){return n.createVertexArray()}function u(S){return n.bindVertexArray(S)}function l(S){return n.deleteVertexArray(S)}function h(S,I,k){let V=k.wireframe===!0,W=i[S.id];W===void 0&&(W={},i[S.id]=W);let j=W[I.id];j===void 0&&(j={},W[I.id]=j);let G=j[V];return G===void 0&&(G=f(c()),j[V]=G),G}function f(S){let I=[],k=[],V=[];for(let W=0;W<t;W++)I[W]=0,k[W]=0,V[W]=0;return{geometry:null,program:null,wireframe:!1,newAttributes:I,enabledAttributes:k,attributeDivisors:V,object:S,attributes:{},index:null}}function p(S,I,k,V){let W=s.attributes,j=I.attributes,G=0,ne=k.getAttributes();for(let z in ne)if(ne[z].location>=0){let ue=W[z],we=j[z];if(we===void 0&&(z==="instanceMatrix"&&S.instanceMatrix&&(we=S.instanceMatrix),z==="instanceColor"&&S.instanceColor&&(we=S.instanceColor)),ue===void 0||ue.attribute!==we||we&&ue.data!==we.data)return!0;G++}return s.attributesNum!==G||s.index!==V}function g(S,I,k,V){let W={},j=I.attributes,G=0,ne=k.getAttributes();for(let z in ne)if(ne[z].location>=0){let ue=j[z];ue===void 0&&(z==="instanceMatrix"&&S.instanceMatrix&&(ue=S.instanceMatrix),z==="instanceColor"&&S.instanceColor&&(ue=S.instanceColor));let we={};we.attribute=ue,ue&&ue.data&&(we.data=ue.data),W[z]=we,G++}s.attributes=W,s.attributesNum=G,s.index=V}function _(){let S=s.newAttributes;for(let I=0,k=S.length;I<k;I++)S[I]=0}function m(S){d(S,0)}function d(S,I){let k=s.newAttributes,V=s.enabledAttributes,W=s.attributeDivisors;k[S]=1,V[S]===0&&(n.enableVertexAttribArray(S),V[S]=1),W[S]!==I&&(n.vertexAttribDivisor(S,I),W[S]=I)}function w(){let S=s.newAttributes,I=s.enabledAttributes;for(let k=0,V=I.length;k<V;k++)I[k]!==S[k]&&(n.disableVertexAttribArray(k),I[k]=0)}function M(S,I,k,V,W,j,G){G===!0?n.vertexAttribIPointer(S,I,k,W,j):n.vertexAttribPointer(S,I,k,V,W,j)}function x(S,I,k,V){_();let W=V.attributes,j=k.getAttributes(),G=I.defaultAttributeValues;for(let ne in j){let z=j[ne];if(z.location>=0){let ae=W[ne];if(ae===void 0&&(ne==="instanceMatrix"&&S.instanceMatrix&&(ae=S.instanceMatrix),ne==="instanceColor"&&S.instanceColor&&(ae=S.instanceColor)),ae!==void 0){let ue=ae.normalized,we=ae.itemSize,je=e.get(ae);if(je===void 0)continue;let ft=je.buffer,xt=je.type,rt=je.bytesPerElement,q=xt===n.INT||xt===n.UNSIGNED_INT||ae.gpuType===Go;if(ae.isInterleavedBufferAttribute){let Z=ae.data,fe=Z.stride,Ne=ae.offset;if(Z.isInstancedInterleavedBuffer){for(let Ee=0;Ee<z.locationSize;Ee++)d(z.location+Ee,Z.meshPerAttribute);S.isInstancedMesh!==!0&&V._maxInstanceCount===void 0&&(V._maxInstanceCount=Z.meshPerAttribute*Z.count)}else for(let Ee=0;Ee<z.locationSize;Ee++)m(z.location+Ee);n.bindBuffer(n.ARRAY_BUFFER,ft);for(let Ee=0;Ee<z.locationSize;Ee++)M(z.location+Ee,we/z.locationSize,xt,ue,fe*rt,(Ne+we/z.locationSize*Ee)*rt,q)}else{if(ae.isInstancedBufferAttribute){for(let Z=0;Z<z.locationSize;Z++)d(z.location+Z,ae.meshPerAttribute);S.isInstancedMesh!==!0&&V._maxInstanceCount===void 0&&(V._maxInstanceCount=ae.meshPerAttribute*ae.count)}else for(let Z=0;Z<z.locationSize;Z++)m(z.location+Z);n.bindBuffer(n.ARRAY_BUFFER,ft);for(let Z=0;Z<z.locationSize;Z++)M(z.location+Z,we/z.locationSize,xt,ue,we*rt,we/z.locationSize*Z*rt,q)}}else if(G!==void 0){let ue=G[ne];if(ue!==void 0)switch(ue.length){case 2:n.vertexAttrib2fv(z.location,ue);break;case 3:n.vertexAttrib3fv(z.location,ue);break;case 4:n.vertexAttrib4fv(z.location,ue);break;default:n.vertexAttrib1fv(z.location,ue)}}}}w()}function T(){O();for(let S in i){let I=i[S];for(let k in I){let V=I[k];for(let W in V)l(V[W].object),delete V[W];delete I[k]}delete i[S]}}function R(S){if(i[S.id]===void 0)return;let I=i[S.id];for(let k in I){let V=I[k];for(let W in V)l(V[W].object),delete V[W];delete I[k]}delete i[S.id]}function P(S){for(let I in i){let k=i[I];if(k[S.id]===void 0)continue;let V=k[S.id];for(let W in V)l(V[W].object),delete V[W];delete k[S.id]}}function O(){b(),a=!0,s!==r&&(s=r,u(s.object))}function b(){r.geometry=null,r.program=null,r.wireframe=!1}return{setup:o,reset:O,resetDefaultState:b,dispose:T,releaseStatesOfGeometry:R,releaseStatesOfProgram:P,initAttributes:_,enableAttribute:m,disableUnusedAttributes:w}}function AS(n,e,t){let i;function r(u){i=u}function s(u,l){n.drawArrays(i,u,l),t.update(l,i,1)}function a(u,l,h){h!==0&&(n.drawArraysInstanced(i,u,l,h),t.update(l,i,h))}function o(u,l,h){if(h===0)return;e.get("WEBGL_multi_draw").multiDrawArraysWEBGL(i,u,0,l,0,h);let p=0;for(let g=0;g<h;g++)p+=l[g];t.update(p,i,1)}function c(u,l,h,f){if(h===0)return;let p=e.get("WEBGL_multi_draw");if(p===null)for(let g=0;g<u.length;g++)a(u[g],l[g],f[g]);else{p.multiDrawArraysInstancedWEBGL(i,u,0,l,0,f,0,h);let g=0;for(let _=0;_<h;_++)g+=l[_]*f[_];t.update(g,i,1)}}this.setMode=r,this.render=s,this.renderInstances=a,this.renderMultiDraw=o,this.renderMultiDrawInstances=c}function RS(n,e,t,i){let r;function s(){if(r!==void 0)return r;if(e.has("EXT_texture_filter_anisotropic")===!0){let P=e.get("EXT_texture_filter_anisotropic");r=n.getParameter(P.MAX_TEXTURE_MAX_ANISOTROPY_EXT)}else r=0;return r}function a(P){return!(P!==Tn&&i.convert(P)!==n.getParameter(n.IMPLEMENTATION_COLOR_READ_FORMAT))}function o(P){let O=P===Qr&&(e.has("EXT_color_buffer_half_float")||e.has("EXT_color_buffer_float"));return!(P!==Zn&&i.convert(P)!==n.getParameter(n.IMPLEMENTATION_COLOR_READ_TYPE)&&P!==Bn&&!O)}function c(P){if(P==="highp"){if(n.getShaderPrecisionFormat(n.VERTEX_SHADER,n.HIGH_FLOAT).precision>0&&n.getShaderPrecisionFormat(n.FRAGMENT_SHADER,n.HIGH_FLOAT).precision>0)return"highp";P="mediump"}return P==="mediump"&&n.getShaderPrecisionFormat(n.VERTEX_SHADER,n.MEDIUM_FLOAT).precision>0&&n.getShaderPrecisionFormat(n.FRAGMENT_SHADER,n.MEDIUM_FLOAT).precision>0?"mediump":"lowp"}let u=t.precision!==void 0?t.precision:"highp",l=c(u);l!==u&&(console.warn("THREE.WebGLRenderer:",u,"not supported, using",l,"instead."),u=l);let h=t.logarithmicDepthBuffer===!0,f=t.reversedDepthBuffer===!0&&e.has("EXT_clip_control"),p=n.getParameter(n.MAX_TEXTURE_IMAGE_UNITS),g=n.getParameter(n.MAX_VERTEX_TEXTURE_IMAGE_UNITS),_=n.getParameter(n.MAX_TEXTURE_SIZE),m=n.getParameter(n.MAX_CUBE_MAP_TEXTURE_SIZE),d=n.getParameter(n.MAX_VERTEX_ATTRIBS),w=n.getParameter(n.MAX_VERTEX_UNIFORM_VECTORS),M=n.getParameter(n.MAX_VARYING_VECTORS),x=n.getParameter(n.MAX_FRAGMENT_UNIFORM_VECTORS),T=g>0,R=n.getParameter(n.MAX_SAMPLES);return{isWebGL2:!0,getMaxAnisotropy:s,getMaxPrecision:c,textureFormatReadable:a,textureTypeReadable:o,precision:u,logarithmicDepthBuffer:h,reversedDepthBuffer:f,maxTextures:p,maxVertexTextures:g,maxTextureSize:_,maxCubemapSize:m,maxAttributes:d,maxVertexUniforms:w,maxVaryings:M,maxFragmentUniforms:x,vertexTextures:T,maxSamples:R}}function CS(n){let e=this,t=null,i=0,r=!1,s=!1,a=new wn,o=new Fe,c={value:null,needsUpdate:!1};this.uniform=c,this.numPlanes=0,this.numIntersection=0,this.init=function(h,f){let p=h.length!==0||f||i!==0||r;return r=f,i=h.length,p},this.beginShadows=function(){s=!0,l(null)},this.endShadows=function(){s=!1},this.setGlobalState=function(h,f){t=l(h,f,0)},this.setState=function(h,f,p){let g=h.clippingPlanes,_=h.clipIntersection,m=h.clipShadows,d=n.get(h);if(!r||g===null||g.length===0||s&&!m)s?l(null):u();else{let w=s?0:i,M=w*4,x=d.clippingState||null;c.value=x,x=l(g,f,M,p);for(let T=0;T!==M;++T)x[T]=t[T];d.clippingState=x,this.numIntersection=_?this.numPlanes:0,this.numPlanes+=w}};function u(){c.value!==t&&(c.value=t,c.needsUpdate=i>0),e.numPlanes=i,e.numIntersection=0}function l(h,f,p,g){let _=h!==null?h.length:0,m=null;if(_!==0){if(m=c.value,g!==!0||m===null){let d=p+_*4,w=f.matrixWorldInverse;o.getNormalMatrix(w),(m===null||m.length<d)&&(m=new Float32Array(d));for(let M=0,x=p;M!==_;++M,x+=4)a.copy(h[M]).applyMatrix4(w,o),a.normal.toArray(m,x),m[x+3]=a.constant}c.value=m,c.needsUpdate=!0}return e.numPlanes=_,e.numIntersection=0,m}}function PS(n){let e=new WeakMap;function t(a,o){return o===Vo?a.mapping=ir:o===$o&&(a.mapping=rr),a}function i(a){if(a&&a.isTexture){let o=a.mapping;if(o===Vo||o===$o)if(e.has(a)){let c=e.get(a).texture;return t(c,a.mapping)}else{let c=a.image;if(c&&c.height>0){let u=new vo(c.height);return u.fromEquirectangularTexture(n,a),e.set(a,u),a.addEventListener("dispose",r),t(u.texture,a.mapping)}else return null}}return a}function r(a){let o=a.target;o.removeEventListener("dispose",r);let c=e.get(o);c!==void 0&&(e.delete(o),c.dispose())}function s(){e=new WeakMap}return{get:i,dispose:s}}var rs=4,Ep=[.125,.215,.35,.446,.526,.582],lr=20,bu=new Do,wp=new Qe,Mu=null,Eu=0,wu=0,Tu=!1,cr=(1+Math.sqrt(5))/2,is=1/cr,Tp=[new L(-cr,is,0),new L(cr,is,0),new L(-is,0,cr),new L(is,0,cr),new L(0,cr,-is),new L(0,cr,is),new L(-1,1,-1),new L(1,1,-1),new L(-1,1,1),new L(1,1,1)],IS=new L,Rc=class{constructor(e){this._renderer=e,this._pingPongRenderTarget=null,this._lodMax=0,this._cubeSize=0,this._lodPlanes=[],this._sizeLods=[],this._sigmas=[],this._blurMaterial=null,this._cubemapMaterial=null,this._equirectMaterial=null,this._compileMaterial(this._blurMaterial)}fromScene(e,t=0,i=.1,r=100,s={}){let{size:a=256,position:o=IS}=s;Mu=this._renderer.getRenderTarget(),Eu=this._renderer.getActiveCubeFace(),wu=this._renderer.getActiveMipmapLevel(),Tu=this._renderer.xr.enabled,this._renderer.xr.enabled=!1,this._setSize(a);let c=this._allocateTargets();return c.depthBuffer=!0,this._sceneToCubeUV(e,i,r,c,o),t>0&&this._blur(c,0,0,t),this._applyPMREM(c),this._cleanup(c),c}fromEquirectangular(e,t=null){return this._fromTexture(e,t)}fromCubemap(e,t=null){return this._fromTexture(e,t)}compileCubemapShader(){this._cubemapMaterial===null&&(this._cubemapMaterial=Cp(),this._compileMaterial(this._cubemapMaterial))}compileEquirectangularShader(){this._equirectMaterial===null&&(this._equirectMaterial=Rp(),this._compileMaterial(this._equirectMaterial))}dispose(){this._dispose(),this._cubemapMaterial!==null&&this._cubemapMaterial.dispose(),this._equirectMaterial!==null&&this._equirectMaterial.dispose()}_setSize(e){this._lodMax=Math.floor(Math.log2(e)),this._cubeSize=Math.pow(2,this._lodMax)}_dispose(){this._blurMaterial!==null&&this._blurMaterial.dispose(),this._pingPongRenderTarget!==null&&this._pingPongRenderTarget.dispose();for(let e=0;e<this._lodPlanes.length;e++)this._lodPlanes[e].dispose()}_cleanup(e){this._renderer.setRenderTarget(Mu,Eu,wu),this._renderer.xr.enabled=Tu,e.scissorTest=!1,Tc(e,0,0,e.width,e.height)}_fromTexture(e,t){e.mapping===ir||e.mapping===rr?this._setSize(e.image.length===0?16:e.image[0].width||e.image[0].image.width):this._setSize(e.image.width/4),Mu=this._renderer.getRenderTarget(),Eu=this._renderer.getActiveCubeFace(),wu=this._renderer.getActiveMipmapLevel(),Tu=this._renderer.xr.enabled,this._renderer.xr.enabled=!1;let i=t||this._allocateTargets();return this._textureToCubeUV(e,i),this._applyPMREM(i),this._cleanup(i),i}_allocateTargets(){let e=3*Math.max(this._cubeSize,112),t=4*this._cubeSize,i={magFilter:On,minFilter:On,generateMipmaps:!1,type:Qr,format:Tn,colorSpace:Qi,depthBuffer:!1},r=Ap(e,t,i);if(this._pingPongRenderTarget===null||this._pingPongRenderTarget.width!==e||this._pingPongRenderTarget.height!==t){this._pingPongRenderTarget!==null&&this._dispose(),this._pingPongRenderTarget=Ap(e,t,i);let{_lodMax:s}=this;({sizeLods:this._sizeLods,lodPlanes:this._lodPlanes,sigmas:this._sigmas}=DS(s)),this._blurMaterial=NS(s,e,t)}return r}_compileMaterial(e){let t=new Ht(this._lodPlanes[0],e);this._renderer.compile(t,bu)}_sceneToCubeUV(e,t,i,r,s){let c=new $t(90,1,t,i),u=[1,-1,1,1,1,1],l=[1,1,1,-1,-1,-1],h=this._renderer,f=h.autoClear,p=h.toneMapping;h.getClearColor(wp),h.toneMapping=hi,h.autoClear=!1,h.state.buffers.depth.getReversed()&&(h.setRenderTarget(r),h.clearDepth(),h.setRenderTarget(null));let _=new Ai({name:"PMREM.Background",side:Jt,depthWrite:!1,depthTest:!1}),m=new Ht(new Yr,_),d=!1,w=e.background;w?w.isColor&&(_.color.copy(w),e.background=null,d=!0):(_.color.copy(wp),d=!0);for(let M=0;M<6;M++){let x=M%3;x===0?(c.up.set(0,u[M],0),c.position.set(s.x,s.y,s.z),c.lookAt(s.x+l[M],s.y,s.z)):x===1?(c.up.set(0,0,u[M]),c.position.set(s.x,s.y,s.z),c.lookAt(s.x,s.y+l[M],s.z)):(c.up.set(0,u[M],0),c.position.set(s.x,s.y,s.z),c.lookAt(s.x,s.y,s.z+l[M]));let T=this._cubeSize;Tc(r,x*T,M>2?T:0,T,T),h.setRenderTarget(r),d&&h.render(m,c),h.render(e,c)}m.geometry.dispose(),m.material.dispose(),h.toneMapping=p,h.autoClear=f,e.background=w}_textureToCubeUV(e,t){let i=this._renderer,r=e.mapping===ir||e.mapping===rr;r?(this._cubemapMaterial===null&&(this._cubemapMaterial=Cp()),this._cubemapMaterial.uniforms.flipEnvMap.value=e.isRenderTargetTexture===!1?-1:1):this._equirectMaterial===null&&(this._equirectMaterial=Rp());let s=r?this._cubemapMaterial:this._equirectMaterial,a=new Ht(this._lodPlanes[0],s),o=s.uniforms;o.envMap.value=e;let c=this._cubeSize;Tc(t,0,0,3*c,2*c),i.setRenderTarget(t),i.render(a,bu)}_applyPMREM(e){let t=this._renderer,i=t.autoClear;t.autoClear=!1;let r=this._lodPlanes.length;for(let s=1;s<r;s++){let a=Math.sqrt(this._sigmas[s]*this._sigmas[s]-this._sigmas[s-1]*this._sigmas[s-1]),o=Tp[(r-s-1)%Tp.length];this._blur(e,s-1,s,a,o)}t.autoClear=i}_blur(e,t,i,r,s){let a=this._pingPongRenderTarget;this._halfBlur(e,a,t,i,r,"latitudinal",s),this._halfBlur(a,e,i,i,r,"longitudinal",s)}_halfBlur(e,t,i,r,s,a,o){let c=this._renderer,u=this._blurMaterial;a!=="latitudinal"&&a!=="longitudinal"&&console.error("blur direction must be either latitudinal or longitudinal!");let l=3,h=new Ht(this._lodPlanes[r],u),f=u.uniforms,p=this._sizeLods[i]-1,g=isFinite(s)?Math.PI/(2*p):2*Math.PI/(2*lr-1),_=s/g,m=isFinite(s)?1+Math.floor(l*_):lr;m>lr&&console.warn(`sigmaRadians, ${s}, is too large and will clip, as it requested ${m} samples when the maximum is set to ${lr}`);let d=[],w=0;for(let P=0;P<lr;++P){let O=P/_,b=Math.exp(-O*O/2);d.push(b),P===0?w+=b:P<m&&(w+=2*b)}for(let P=0;P<d.length;P++)d[P]=d[P]/w;f.envMap.value=e.texture,f.samples.value=m,f.weights.value=d,f.latitudinal.value=a==="latitudinal",o&&(f.poleAxis.value=o);let{_lodMax:M}=this;f.dTheta.value=g,f.mipInt.value=M-i;let x=this._sizeLods[r],T=3*x*(r>M-rs?r-M+rs:0),R=4*(this._cubeSize-x);Tc(t,T,R,3*x,2*x),c.setRenderTarget(t),c.render(h,bu)}};function DS(n){let e=[],t=[],i=[],r=n,s=n-rs+1+Ep.length;for(let a=0;a<s;a++){let o=Math.pow(2,r);t.push(o);let c=1/o;a>n-rs?c=Ep[a-n+rs-1]:a===0&&(c=0),i.push(c);let u=1/(o-2),l=-u,h=1+u,f=[l,l,h,l,h,h,l,l,h,h,l,h],p=6,g=6,_=3,m=2,d=1,w=new Float32Array(_*g*p),M=new Float32Array(m*g*p),x=new Float32Array(d*g*p);for(let R=0;R<p;R++){let P=R%3*2/3-1,O=R>2?0:-1,b=[P,O,0,P+2/3,O,0,P+2/3,O+1,0,P,O,0,P+2/3,O+1,0,P,O+1,0];w.set(b,_*g*R),M.set(f,m*g*R);let S=[R,R,R,R,R,R];x.set(S,d*g*R)}let T=new li;T.setAttribute("position",new rn(w,_)),T.setAttribute("uv",new rn(M,m)),T.setAttribute("faceIndex",new rn(x,d)),e.push(T),r>rs&&r--}return{lodPlanes:e,sizeLods:t,sigmas:i}}function Ap(n,e,t){let i=new Wn(n,e,t);return i.texture.mapping=Qs,i.texture.name="PMREM.cubeUv",i.scissorTest=!0,i}function Tc(n,e,t,i,r){n.viewport.set(e,t,i,r),n.scissor.set(e,t,i,r)}function NS(n,e,t){let i=new Float32Array(lr),r=new L(0,1,0);return new Fn({name:"SphericalGaussianBlur",defines:{n:lr,CUBEUV_TEXEL_WIDTH:1/e,CUBEUV_TEXEL_HEIGHT:1/t,CUBEUV_MAX_MIP:`${n}.0`},uniforms:{envMap:{value:null},samples:{value:1},weights:{value:i},latitudinal:{value:!1},dTheta:{value:0},mipInt:{value:0},poleAxis:{value:r}},vertexShader:Ou(),fragmentShader:`

			precision mediump float;
			precision mediump int;

			varying vec3 vOutputDirection;

			uniform sampler2D envMap;
			uniform int samples;
			uniform float weights[ n ];
			uniform bool latitudinal;
			uniform float dTheta;
			uniform float mipInt;
			uniform vec3 poleAxis;

			#define ENVMAP_TYPE_CUBE_UV
			#include <cube_uv_reflection_fragment>

			vec3 getSample( float theta, vec3 axis ) {

				float cosTheta = cos( theta );
				// Rodrigues' axis-angle rotation
				vec3 sampleDirection = vOutputDirection * cosTheta
					+ cross( axis, vOutputDirection ) * sin( theta )
					+ axis * dot( axis, vOutputDirection ) * ( 1.0 - cosTheta );

				return bilinearCubeUV( envMap, sampleDirection, mipInt );

			}

			void main() {

				vec3 axis = latitudinal ? poleAxis : cross( poleAxis, vOutputDirection );

				if ( all( equal( axis, vec3( 0.0 ) ) ) ) {

					axis = vec3( vOutputDirection.z, 0.0, - vOutputDirection.x );

				}

				axis = normalize( axis );

				gl_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
				gl_FragColor.rgb += weights[ 0 ] * getSample( 0.0, axis );

				for ( int i = 1; i < n; i++ ) {

					if ( i >= samples ) {

						break;

					}

					float theta = dTheta * float( i );
					gl_FragColor.rgb += weights[ i ] * getSample( -1.0 * theta, axis );
					gl_FragColor.rgb += weights[ i ] * getSample( theta, axis );

				}

			}
		`,blending:ui,depthTest:!1,depthWrite:!1})}function Rp(){return new Fn({name:"EquirectangularToCubeUV",uniforms:{envMap:{value:null}},vertexShader:Ou(),fragmentShader:`

			precision mediump float;
			precision mediump int;

			varying vec3 vOutputDirection;

			uniform sampler2D envMap;

			#include <common>

			void main() {

				vec3 outputDirection = normalize( vOutputDirection );
				vec2 uv = equirectUv( outputDirection );

				gl_FragColor = vec4( texture2D ( envMap, uv ).rgb, 1.0 );

			}
		`,blending:ui,depthTest:!1,depthWrite:!1})}function Cp(){return new Fn({name:"CubemapToCubeUV",uniforms:{envMap:{value:null},flipEnvMap:{value:-1}},vertexShader:Ou(),fragmentShader:`

			precision mediump float;
			precision mediump int;

			uniform float flipEnvMap;

			varying vec3 vOutputDirection;

			uniform samplerCube envMap;

			void main() {

				gl_FragColor = textureCube( envMap, vec3( flipEnvMap * vOutputDirection.x, vOutputDirection.yz ) );

			}
		`,blending:ui,depthTest:!1,depthWrite:!1})}function Ou(){return`

		precision mediump float;
		precision mediump int;

		attribute float faceIndex;

		varying vec3 vOutputDirection;

		// RH coordinate system; PMREM face-indexing convention
		vec3 getDirection( vec2 uv, float face ) {

			uv = 2.0 * uv - 1.0;

			vec3 direction = vec3( uv, 1.0 );

			if ( face == 0.0 ) {

				direction = direction.zyx; // ( 1, v, u ) pos x

			} else if ( face == 1.0 ) {

				direction = direction.xzy;
				direction.xz *= -1.0; // ( -u, 1, -v ) pos y

			} else if ( face == 2.0 ) {

				direction.x *= -1.0; // ( -u, v, 1 ) pos z

			} else if ( face == 3.0 ) {

				direction = direction.zyx;
				direction.xz *= -1.0; // ( -1, v, -u ) neg x

			} else if ( face == 4.0 ) {

				direction = direction.xzy;
				direction.xy *= -1.0; // ( -u, -1, v ) neg y

			} else if ( face == 5.0 ) {

				direction.z *= -1.0; // ( u, v, -1 ) neg z

			}

			return direction;

		}

		void main() {

			vOutputDirection = getDirection( uv, faceIndex );
			gl_Position = vec4( position, 1.0 );

		}
	`}function LS(n){let e=new WeakMap,t=null;function i(o){if(o&&o.isTexture){let c=o.mapping,u=c===Vo||c===$o,l=c===ir||c===rr;if(u||l){let h=e.get(o),f=h!==void 0?h.texture.pmremVersion:0;if(o.isRenderTargetTexture&&o.pmremVersion!==f)return t===null&&(t=new Rc(n)),h=u?t.fromEquirectangular(o,h):t.fromCubemap(o,h),h.texture.pmremVersion=o.pmremVersion,e.set(o,h),h.texture;if(h!==void 0)return h.texture;{let p=o.image;return u&&p&&p.height>0||l&&p&&r(p)?(t===null&&(t=new Rc(n)),h=u?t.fromEquirectangular(o):t.fromCubemap(o),h.texture.pmremVersion=o.pmremVersion,e.set(o,h),o.addEventListener("dispose",s),h.texture):null}}}return o}function r(o){let c=0,u=6;for(let l=0;l<u;l++)o[l]!==void 0&&c++;return c===u}function s(o){let c=o.target;c.removeEventListener("dispose",s);let u=e.get(c);u!==void 0&&(e.delete(c),u.dispose())}function a(){e=new WeakMap,t!==null&&(t.dispose(),t=null)}return{get:i,dispose:a}}function US(n){let e={};function t(i){if(e[i]!==void 0)return e[i];let r;switch(i){case"WEBGL_depth_texture":r=n.getExtension("WEBGL_depth_texture")||n.getExtension("MOZ_WEBGL_depth_texture")||n.getExtension("WEBKIT_WEBGL_depth_texture");break;case"EXT_texture_filter_anisotropic":r=n.getExtension("EXT_texture_filter_anisotropic")||n.getExtension("MOZ_EXT_texture_filter_anisotropic")||n.getExtension("WEBKIT_EXT_texture_filter_anisotropic");break;case"WEBGL_compressed_texture_s3tc":r=n.getExtension("WEBGL_compressed_texture_s3tc")||n.getExtension("MOZ_WEBGL_compressed_texture_s3tc")||n.getExtension("WEBKIT_WEBGL_compressed_texture_s3tc");break;case"WEBGL_compressed_texture_pvrtc":r=n.getExtension("WEBGL_compressed_texture_pvrtc")||n.getExtension("WEBKIT_WEBGL_compressed_texture_pvrtc");break;default:r=n.getExtension(i)}return e[i]=r,r}return{has:function(i){return t(i)!==null},init:function(){t("EXT_color_buffer_float"),t("WEBGL_clip_cull_distance"),t("OES_texture_float_linear"),t("EXT_color_buffer_half_float"),t("WEBGL_multisampled_render_to_texture"),t("WEBGL_render_shared_exponent")},get:function(i){let r=t(i);return r===null&&qr("THREE.WebGLRenderer: "+i+" extension not supported."),r}}}function OS(n,e,t,i){let r={},s=new WeakMap;function a(h){let f=h.target;f.index!==null&&e.remove(f.index);for(let g in f.attributes)e.remove(f.attributes[g]);f.removeEventListener("dispose",a),delete r[f.id];let p=s.get(f);p&&(e.remove(p),s.delete(f)),i.releaseStatesOfGeometry(f),f.isInstancedBufferGeometry===!0&&delete f._maxInstanceCount,t.memory.geometries--}function o(h,f){return r[f.id]===!0||(f.addEventListener("dispose",a),r[f.id]=!0,t.memory.geometries++),f}function c(h){let f=h.attributes;for(let p in f)e.update(f[p],n.ARRAY_BUFFER)}function u(h){let f=[],p=h.index,g=h.attributes.position,_=0;if(p!==null){let w=p.array;_=p.version;for(let M=0,x=w.length;M<x;M+=3){let T=w[M+0],R=w[M+1],P=w[M+2];f.push(T,R,R,P,P,T)}}else if(g!==void 0){let w=g.array;_=g.version;for(let M=0,x=w.length/3-1;M<x;M+=3){let T=M+0,R=M+1,P=M+2;f.push(T,R,R,P,P,T)}}else return;let m=new(_u(f)?ks:Bs)(f,1);m.version=_;let d=s.get(h);d&&e.remove(d),s.set(h,m)}function l(h){let f=s.get(h);if(f){let p=h.index;p!==null&&f.version<p.version&&u(h)}else u(h);return s.get(h)}return{get:o,update:c,getWireframeAttribute:l}}function FS(n,e,t){let i;function r(f){i=f}let s,a;function o(f){s=f.type,a=f.bytesPerElement}function c(f,p){n.drawElements(i,p,s,f*a),t.update(p,i,1)}function u(f,p,g){g!==0&&(n.drawElementsInstanced(i,p,s,f*a,g),t.update(p,i,g))}function l(f,p,g){if(g===0)return;e.get("WEBGL_multi_draw").multiDrawElementsWEBGL(i,p,0,s,f,0,g);let m=0;for(let d=0;d<g;d++)m+=p[d];t.update(m,i,1)}function h(f,p,g,_){if(g===0)return;let m=e.get("WEBGL_multi_draw");if(m===null)for(let d=0;d<f.length;d++)u(f[d]/a,p[d],_[d]);else{m.multiDrawElementsInstancedWEBGL(i,p,0,s,f,0,_,0,g);let d=0;for(let w=0;w<g;w++)d+=p[w]*_[w];t.update(d,i,1)}}this.setMode=r,this.setIndex=o,this.render=c,this.renderInstances=u,this.renderMultiDraw=l,this.renderMultiDrawInstances=h}function BS(n){let e={geometries:0,textures:0},t={frame:0,calls:0,triangles:0,points:0,lines:0};function i(s,a,o){switch(t.calls++,a){case n.TRIANGLES:t.triangles+=o*(s/3);break;case n.LINES:t.lines+=o*(s/2);break;case n.LINE_STRIP:t.lines+=o*(s-1);break;case n.LINE_LOOP:t.lines+=o*s;break;case n.POINTS:t.points+=o*s;break;default:console.error("THREE.WebGLInfo: Unknown draw mode:",a);break}}function r(){t.calls=0,t.triangles=0,t.points=0,t.lines=0}return{memory:e,render:t,programs:null,autoReset:!0,reset:r,update:i}}function kS(n,e,t){let i=new WeakMap,r=new Tt;function s(a,o,c){let u=a.morphTargetInfluences,l=o.morphAttributes.position||o.morphAttributes.normal||o.morphAttributes.color,h=l!==void 0?l.length:0,f=i.get(o);if(f===void 0||f.count!==h){let b=function(){P.dispose(),i.delete(o),o.removeEventListener("dispose",b)};f!==void 0&&f.texture.dispose();let p=o.morphAttributes.position!==void 0,g=o.morphAttributes.normal!==void 0,_=o.morphAttributes.color!==void 0,m=o.morphAttributes.position||[],d=o.morphAttributes.normal||[],w=o.morphAttributes.color||[],M=0;p===!0&&(M=1),g===!0&&(M=2),_===!0&&(M=3);let x=o.attributes.position.count*M,T=1;x>e.maxTextureSize&&(T=Math.ceil(x/e.maxTextureSize),x=e.maxTextureSize);let R=new Float32Array(x*T*4*h),P=new Fs(R,x,T,h);P.type=Bn,P.needsUpdate=!0;let O=M*4;for(let S=0;S<h;S++){let I=m[S],k=d[S],V=w[S],W=x*T*4*S;for(let j=0;j<I.count;j++){let G=j*O;p===!0&&(r.fromBufferAttribute(I,j),R[W+G+0]=r.x,R[W+G+1]=r.y,R[W+G+2]=r.z,R[W+G+3]=0),g===!0&&(r.fromBufferAttribute(k,j),R[W+G+4]=r.x,R[W+G+5]=r.y,R[W+G+6]=r.z,R[W+G+7]=0),_===!0&&(r.fromBufferAttribute(V,j),R[W+G+8]=r.x,R[W+G+9]=r.y,R[W+G+10]=r.z,R[W+G+11]=V.itemSize===4?r.w:1)}}f={count:h,texture:P,size:new Ue(x,T)},i.set(o,f),o.addEventListener("dispose",b)}if(a.isInstancedMesh===!0&&a.morphTexture!==null)c.getUniforms().setValue(n,"morphTexture",a.morphTexture,t);else{let p=0;for(let _=0;_<u.length;_++)p+=u[_];let g=o.morphTargetsRelative?1:1-p;c.getUniforms().setValue(n,"morphTargetBaseInfluence",g),c.getUniforms().setValue(n,"morphTargetInfluences",u)}c.getUniforms().setValue(n,"morphTargetsTexture",f.texture,t),c.getUniforms().setValue(n,"morphTargetsTextureSize",f.size)}return{update:s}}function zS(n,e,t,i){let r=new WeakMap;function s(c){let u=i.render.frame,l=c.geometry,h=e.get(c,l);if(r.get(h)!==u&&(e.update(h),r.set(h,u)),c.isInstancedMesh&&(c.hasEventListener("dispose",o)===!1&&c.addEventListener("dispose",o),r.get(c)!==u&&(t.update(c.instanceMatrix,n.ARRAY_BUFFER),c.instanceColor!==null&&t.update(c.instanceColor,n.ARRAY_BUFFER),r.set(c,u))),c.isSkinnedMesh){let f=c.skeleton;r.get(f)!==u&&(f.update(),r.set(f,u))}return h}function a(){r=new WeakMap}function o(c){let u=c.target;u.removeEventListener("dispose",o),t.remove(u.instanceMatrix),u.instanceColor!==null&&t.remove(u.instanceColor)}return{update:s,dispose:a}}var jp=new on,Pp=new qs(1,1),Yp=new Fs,Zp=new _o,Kp=new Vs,Ip=[],Dp=[],Np=new Float32Array(16),Lp=new Float32Array(9),Up=new Float32Array(4);function as(n,e,t){let i=n[0];if(i<=0||i>0)return n;let r=e*t,s=Ip[r];if(s===void 0&&(s=new Float32Array(r),Ip[r]=s),e!==0){i.toArray(s,0);for(let a=1,o=0;a!==e;++a)o+=t,n[a].toArray(s,o)}return s}function Pt(n,e){if(n.length!==e.length)return!1;for(let t=0,i=n.length;t<i;t++)if(n[t]!==e[t])return!1;return!0}function It(n,e){for(let t=0,i=e.length;t<i;t++)n[t]=e[t]}function Pc(n,e){let t=Dp[e];t===void 0&&(t=new Int32Array(e),Dp[e]=t);for(let i=0;i!==e;++i)t[i]=n.allocateTextureUnit();return t}function VS(n,e){let t=this.cache;t[0]!==e&&(n.uniform1f(this.addr,e),t[0]=e)}function $S(n,e){let t=this.cache;if(e.x!==void 0)(t[0]!==e.x||t[1]!==e.y)&&(n.uniform2f(this.addr,e.x,e.y),t[0]=e.x,t[1]=e.y);else{if(Pt(t,e))return;n.uniform2fv(this.addr,e),It(t,e)}}function HS(n,e){let t=this.cache;if(e.x!==void 0)(t[0]!==e.x||t[1]!==e.y||t[2]!==e.z)&&(n.uniform3f(this.addr,e.x,e.y,e.z),t[0]=e.x,t[1]=e.y,t[2]=e.z);else if(e.r!==void 0)(t[0]!==e.r||t[1]!==e.g||t[2]!==e.b)&&(n.uniform3f(this.addr,e.r,e.g,e.b),t[0]=e.r,t[1]=e.g,t[2]=e.b);else{if(Pt(t,e))return;n.uniform3fv(this.addr,e),It(t,e)}}function GS(n,e){let t=this.cache;if(e.x!==void 0)(t[0]!==e.x||t[1]!==e.y||t[2]!==e.z||t[3]!==e.w)&&(n.uniform4f(this.addr,e.x,e.y,e.z,e.w),t[0]=e.x,t[1]=e.y,t[2]=e.z,t[3]=e.w);else{if(Pt(t,e))return;n.uniform4fv(this.addr,e),It(t,e)}}function WS(n,e){let t=this.cache,i=e.elements;if(i===void 0){if(Pt(t,e))return;n.uniformMatrix2fv(this.addr,!1,e),It(t,e)}else{if(Pt(t,i))return;Up.set(i),n.uniformMatrix2fv(this.addr,!1,Up),It(t,i)}}function qS(n,e){let t=this.cache,i=e.elements;if(i===void 0){if(Pt(t,e))return;n.uniformMatrix3fv(this.addr,!1,e),It(t,e)}else{if(Pt(t,i))return;Lp.set(i),n.uniformMatrix3fv(this.addr,!1,Lp),It(t,i)}}function XS(n,e){let t=this.cache,i=e.elements;if(i===void 0){if(Pt(t,e))return;n.uniformMatrix4fv(this.addr,!1,e),It(t,e)}else{if(Pt(t,i))return;Np.set(i),n.uniformMatrix4fv(this.addr,!1,Np),It(t,i)}}function jS(n,e){let t=this.cache;t[0]!==e&&(n.uniform1i(this.addr,e),t[0]=e)}function YS(n,e){let t=this.cache;if(e.x!==void 0)(t[0]!==e.x||t[1]!==e.y)&&(n.uniform2i(this.addr,e.x,e.y),t[0]=e.x,t[1]=e.y);else{if(Pt(t,e))return;n.uniform2iv(this.addr,e),It(t,e)}}function ZS(n,e){let t=this.cache;if(e.x!==void 0)(t[0]!==e.x||t[1]!==e.y||t[2]!==e.z)&&(n.uniform3i(this.addr,e.x,e.y,e.z),t[0]=e.x,t[1]=e.y,t[2]=e.z);else{if(Pt(t,e))return;n.uniform3iv(this.addr,e),It(t,e)}}function KS(n,e){let t=this.cache;if(e.x!==void 0)(t[0]!==e.x||t[1]!==e.y||t[2]!==e.z||t[3]!==e.w)&&(n.uniform4i(this.addr,e.x,e.y,e.z,e.w),t[0]=e.x,t[1]=e.y,t[2]=e.z,t[3]=e.w);else{if(Pt(t,e))return;n.uniform4iv(this.addr,e),It(t,e)}}function JS(n,e){let t=this.cache;t[0]!==e&&(n.uniform1ui(this.addr,e),t[0]=e)}function QS(n,e){let t=this.cache;if(e.x!==void 0)(t[0]!==e.x||t[1]!==e.y)&&(n.uniform2ui(this.addr,e.x,e.y),t[0]=e.x,t[1]=e.y);else{if(Pt(t,e))return;n.uniform2uiv(this.addr,e),It(t,e)}}function eb(n,e){let t=this.cache;if(e.x!==void 0)(t[0]!==e.x||t[1]!==e.y||t[2]!==e.z)&&(n.uniform3ui(this.addr,e.x,e.y,e.z),t[0]=e.x,t[1]=e.y,t[2]=e.z);else{if(Pt(t,e))return;n.uniform3uiv(this.addr,e),It(t,e)}}function tb(n,e){let t=this.cache;if(e.x!==void 0)(t[0]!==e.x||t[1]!==e.y||t[2]!==e.z||t[3]!==e.w)&&(n.uniform4ui(this.addr,e.x,e.y,e.z,e.w),t[0]=e.x,t[1]=e.y,t[2]=e.z,t[3]=e.w);else{if(Pt(t,e))return;n.uniform4uiv(this.addr,e),It(t,e)}}function nb(n,e,t){let i=this.cache,r=t.allocateTextureUnit();i[0]!==r&&(n.uniform1i(this.addr,r),i[0]=r);let s;this.type===n.SAMPLER_2D_SHADOW?(Pp.compareFunction=pu,s=Pp):s=jp,t.setTexture2D(e||s,r)}function ib(n,e,t){let i=this.cache,r=t.allocateTextureUnit();i[0]!==r&&(n.uniform1i(this.addr,r),i[0]=r),t.setTexture3D(e||Zp,r)}function rb(n,e,t){let i=this.cache,r=t.allocateTextureUnit();i[0]!==r&&(n.uniform1i(this.addr,r),i[0]=r),t.setTextureCube(e||Kp,r)}function sb(n,e,t){let i=this.cache,r=t.allocateTextureUnit();i[0]!==r&&(n.uniform1i(this.addr,r),i[0]=r),t.setTexture2DArray(e||Yp,r)}function ab(n){switch(n){case 5126:return VS;case 35664:return $S;case 35665:return HS;case 35666:return GS;case 35674:return WS;case 35675:return qS;case 35676:return XS;case 5124:case 35670:return jS;case 35667:case 35671:return YS;case 35668:case 35672:return ZS;case 35669:case 35673:return KS;case 5125:return JS;case 36294:return QS;case 36295:return eb;case 36296:return tb;case 35678:case 36198:case 36298:case 36306:case 35682:return nb;case 35679:case 36299:case 36307:return ib;case 35680:case 36300:case 36308:case 36293:return rb;case 36289:case 36303:case 36311:case 36292:return sb}}function ob(n,e){n.uniform1fv(this.addr,e)}function cb(n,e){let t=as(e,this.size,2);n.uniform2fv(this.addr,t)}function lb(n,e){let t=as(e,this.size,3);n.uniform3fv(this.addr,t)}function ub(n,e){let t=as(e,this.size,4);n.uniform4fv(this.addr,t)}function hb(n,e){let t=as(e,this.size,4);n.uniformMatrix2fv(this.addr,!1,t)}function db(n,e){let t=as(e,this.size,9);n.uniformMatrix3fv(this.addr,!1,t)}function fb(n,e){let t=as(e,this.size,16);n.uniformMatrix4fv(this.addr,!1,t)}function pb(n,e){n.uniform1iv(this.addr,e)}function mb(n,e){n.uniform2iv(this.addr,e)}function gb(n,e){n.uniform3iv(this.addr,e)}function _b(n,e){n.uniform4iv(this.addr,e)}function yb(n,e){n.uniform1uiv(this.addr,e)}function vb(n,e){n.uniform2uiv(this.addr,e)}function xb(n,e){n.uniform3uiv(this.addr,e)}function Sb(n,e){n.uniform4uiv(this.addr,e)}function bb(n,e,t){let i=this.cache,r=e.length,s=Pc(t,r);Pt(i,s)||(n.uniform1iv(this.addr,s),It(i,s));for(let a=0;a!==r;++a)t.setTexture2D(e[a]||jp,s[a])}function Mb(n,e,t){let i=this.cache,r=e.length,s=Pc(t,r);Pt(i,s)||(n.uniform1iv(this.addr,s),It(i,s));for(let a=0;a!==r;++a)t.setTexture3D(e[a]||Zp,s[a])}function Eb(n,e,t){let i=this.cache,r=e.length,s=Pc(t,r);Pt(i,s)||(n.uniform1iv(this.addr,s),It(i,s));for(let a=0;a!==r;++a)t.setTextureCube(e[a]||Kp,s[a])}function wb(n,e,t){let i=this.cache,r=e.length,s=Pc(t,r);Pt(i,s)||(n.uniform1iv(this.addr,s),It(i,s));for(let a=0;a!==r;++a)t.setTexture2DArray(e[a]||Yp,s[a])}function Tb(n){switch(n){case 5126:return ob;case 35664:return cb;case 35665:return lb;case 35666:return ub;case 35674:return hb;case 35675:return db;case 35676:return fb;case 5124:case 35670:return pb;case 35667:case 35671:return mb;case 35668:case 35672:return gb;case 35669:case 35673:return _b;case 5125:return yb;case 36294:return vb;case 36295:return xb;case 36296:return Sb;case 35678:case 36198:case 36298:case 36306:case 35682:return bb;case 35679:case 36299:case 36307:return Mb;case 35680:case 36300:case 36308:case 36293:return Eb;case 36289:case 36303:case 36311:case 36292:return wb}}var Ru=class{constructor(e,t,i){this.id=e,this.addr=i,this.cache=[],this.type=t.type,this.setValue=ab(t.type)}},Cu=class{constructor(e,t,i){this.id=e,this.addr=i,this.cache=[],this.type=t.type,this.size=t.size,this.setValue=Tb(t.type)}},Pu=class{constructor(e){this.id=e,this.seq=[],this.map={}}setValue(e,t,i){let r=this.seq;for(let s=0,a=r.length;s!==a;++s){let o=r[s];o.setValue(e,t[o.id],i)}}},Au=/(\w+)(\])?(\[|\.)?/g;function Op(n,e){n.seq.push(e),n.map[e.id]=e}function Ab(n,e,t){let i=n.name,r=i.length;for(Au.lastIndex=0;;){let s=Au.exec(i),a=Au.lastIndex,o=s[1],c=s[2]==="]",u=s[3];if(c&&(o=o|0),u===void 0||u==="["&&a+2===r){Op(t,u===void 0?new Ru(o,n,e):new Cu(o,n,e));break}else{let h=t.map[o];h===void 0&&(h=new Pu(o),Op(t,h)),t=h}}}var ss=class{constructor(e,t){this.seq=[],this.map={};let i=e.getProgramParameter(t,e.ACTIVE_UNIFORMS);for(let r=0;r<i;++r){let s=e.getActiveUniform(t,r),a=e.getUniformLocation(t,s.name);Ab(s,a,this)}}setValue(e,t,i,r){let s=this.map[t];s!==void 0&&s.setValue(e,i,r)}setOptional(e,t,i){let r=t[i];r!==void 0&&this.setValue(e,i,r)}static upload(e,t,i,r){for(let s=0,a=t.length;s!==a;++s){let o=t[s],c=i[o.id];c.needsUpdate!==!1&&o.setValue(e,c.value,r)}}static seqWithValue(e,t){let i=[];for(let r=0,s=e.length;r!==s;++r){let a=e[r];a.id in t&&i.push(a)}return i}};function Fp(n,e,t){let i=n.createShader(e);return n.shaderSource(i,t),n.compileShader(i),i}var Rb=37297,Cb=0;function Pb(n,e){let t=n.split(`
`),i=[],r=Math.max(e-6,0),s=Math.min(e+6,t.length);for(let a=r;a<s;a++){let o=a+1;i.push(`${o===e?">":" "} ${o}: ${t[a]}`)}return i.join(`
`)}var Bp=new Fe;function Ib(n){tt._getMatrix(Bp,tt.workingColorSpace,n);let e=`mat3( ${Bp.elements.map(t=>t.toFixed(4))} )`;switch(tt.getTransfer(n)){case Ls:return[e,"LinearTransferOETF"];case ot:return[e,"sRGBTransferOETF"];default:return console.warn("THREE.WebGLProgram: Unsupported color space: ",n),[e,"LinearTransferOETF"]}}function kp(n,e,t){let i=n.getShaderParameter(e,n.COMPILE_STATUS),s=(n.getShaderInfoLog(e)||"").trim();if(i&&s==="")return"";let a=/ERROR: 0:(\d+)/.exec(s);if(a){let o=parseInt(a[1]);return t.toUpperCase()+`

`+s+`

`+Pb(n.getShaderSource(e),o)}else return s}function Db(n,e){let t=Ib(e);return[`vec4 ${n}( vec4 value ) {`,`	return ${t[1]}( vec4( value.rgb * ${t[0]}, value.a ) );`,"}"].join(`
`)}function Nb(n,e){let t;switch(e){case ep:t="Linear";break;case tp:t="Reinhard";break;case np:t="Cineon";break;case ip:t="ACESFilmic";break;case sp:t="AgX";break;case ap:t="Neutral";break;case rp:t="Custom";break;default:console.warn("THREE.WebGLProgram: Unsupported toneMapping:",e),t="Linear"}return"vec3 "+n+"( vec3 color ) { return "+t+"ToneMapping( color ); }"}var Ac=new L;function Lb(){tt.getLuminanceCoefficients(Ac);let n=Ac.x.toFixed(4),e=Ac.y.toFixed(4),t=Ac.z.toFixed(4);return["float luminance( const in vec3 rgb ) {",`	const vec3 weights = vec3( ${n}, ${e}, ${t} );`,"	return dot( weights, rgb );","}"].join(`
`)}function Ub(n){return[n.extensionClipCullDistance?"#extension GL_ANGLE_clip_cull_distance : require":"",n.extensionMultiDraw?"#extension GL_ANGLE_multi_draw : require":""].filter(aa).join(`
`)}function Ob(n){let e=[];for(let t in n){let i=n[t];i!==!1&&e.push("#define "+t+" "+i)}return e.join(`
`)}function Fb(n,e){let t={},i=n.getProgramParameter(e,n.ACTIVE_ATTRIBUTES);for(let r=0;r<i;r++){let s=n.getActiveAttrib(e,r),a=s.name,o=1;s.type===n.FLOAT_MAT2&&(o=2),s.type===n.FLOAT_MAT3&&(o=3),s.type===n.FLOAT_MAT4&&(o=4),t[a]={type:s.type,location:n.getAttribLocation(e,a),locationSize:o}}return t}function aa(n){return n!==""}function zp(n,e){let t=e.numSpotLightShadows+e.numSpotLightMaps-e.numSpotLightShadowsWithMaps;return n.replace(/NUM_DIR_LIGHTS/g,e.numDirLights).replace(/NUM_SPOT_LIGHTS/g,e.numSpotLights).replace(/NUM_SPOT_LIGHT_MAPS/g,e.numSpotLightMaps).replace(/NUM_SPOT_LIGHT_COORDS/g,t).replace(/NUM_RECT_AREA_LIGHTS/g,e.numRectAreaLights).replace(/NUM_POINT_LIGHTS/g,e.numPointLights).replace(/NUM_HEMI_LIGHTS/g,e.numHemiLights).replace(/NUM_DIR_LIGHT_SHADOWS/g,e.numDirLightShadows).replace(/NUM_SPOT_LIGHT_SHADOWS_WITH_MAPS/g,e.numSpotLightShadowsWithMaps).replace(/NUM_SPOT_LIGHT_SHADOWS/g,e.numSpotLightShadows).replace(/NUM_POINT_LIGHT_SHADOWS/g,e.numPointLightShadows)}function Vp(n,e){return n.replace(/NUM_CLIPPING_PLANES/g,e.numClippingPlanes).replace(/UNION_CLIPPING_PLANES/g,e.numClippingPlanes-e.numClipIntersection)}var Bb=/^[ \t]*#include +<([\w\d./]+)>/gm;function Iu(n){return n.replace(Bb,zb)}var kb=new Map;function zb(n,e){let t=He[e];if(t===void 0){let i=kb.get(e);if(i!==void 0)t=He[i],console.warn('THREE.WebGLRenderer: Shader chunk "%s" has been deprecated. Use "%s" instead.',e,i);else throw new Error("Can not resolve #include <"+e+">")}return Iu(t)}var Vb=/#pragma unroll_loop_start\s+for\s*\(\s*int\s+i\s*=\s*(\d+)\s*;\s*i\s*<\s*(\d+)\s*;\s*i\s*\+\+\s*\)\s*{([\s\S]+?)}\s+#pragma unroll_loop_end/g;function $p(n){return n.replace(Vb,$b)}function $b(n,e,t,i){let r="";for(let s=parseInt(e);s<parseInt(t);s++)r+=i.replace(/\[\s*i\s*\]/g,"[ "+s+" ]").replace(/UNROLLED_LOOP_INDEX/g,s);return r}function Hp(n){let e=`precision ${n.precision} float;
	precision ${n.precision} int;
	precision ${n.precision} sampler2D;
	precision ${n.precision} samplerCube;
	precision ${n.precision} sampler3D;
	precision ${n.precision} sampler2DArray;
	precision ${n.precision} sampler2DShadow;
	precision ${n.precision} samplerCubeShadow;
	precision ${n.precision} sampler2DArrayShadow;
	precision ${n.precision} isampler2D;
	precision ${n.precision} isampler3D;
	precision ${n.precision} isamplerCube;
	precision ${n.precision} isampler2DArray;
	precision ${n.precision} usampler2D;
	precision ${n.precision} usampler3D;
	precision ${n.precision} usamplerCube;
	precision ${n.precision} usampler2DArray;
	`;return n.precision==="highp"?e+=`
#define HIGH_PRECISION`:n.precision==="mediump"?e+=`
#define MEDIUM_PRECISION`:n.precision==="lowp"&&(e+=`
#define LOW_PRECISION`),e}function Hb(n){let e="SHADOWMAP_TYPE_BASIC";return n.shadowMapType===tu?e="SHADOWMAP_TYPE_PCF":n.shadowMapType===Nf?e="SHADOWMAP_TYPE_PCF_SOFT":n.shadowMapType===jn&&(e="SHADOWMAP_TYPE_VSM"),e}function Gb(n){let e="ENVMAP_TYPE_CUBE";if(n.envMap)switch(n.envMapMode){case ir:case rr:e="ENVMAP_TYPE_CUBE";break;case Qs:e="ENVMAP_TYPE_CUBE_UV";break}return e}function Wb(n){let e="ENVMAP_MODE_REFLECTION";if(n.envMap)switch(n.envMapMode){case rr:e="ENVMAP_MODE_REFRACTION";break}return e}function qb(n){let e="ENVMAP_BLENDING_NONE";if(n.envMap)switch(n.combine){case su:e="ENVMAP_BLENDING_MULTIPLY";break;case Jf:e="ENVMAP_BLENDING_MIX";break;case Qf:e="ENVMAP_BLENDING_ADD";break}return e}function Xb(n){let e=n.envMapCubeUVHeight;if(e===null)return null;let t=Math.log2(e)-2,i=1/e;return{texelWidth:1/(3*Math.max(Math.pow(2,t),112)),texelHeight:i,maxMip:t}}function jb(n,e,t,i){let r=n.getContext(),s=t.defines,a=t.vertexShader,o=t.fragmentShader,c=Hb(t),u=Gb(t),l=Wb(t),h=qb(t),f=Xb(t),p=Ub(t),g=Ob(s),_=r.createProgram(),m,d,w=t.glslVersion?"#version "+t.glslVersion+`
`:"";t.isRawShaderMaterial?(m=["#define SHADER_TYPE "+t.shaderType,"#define SHADER_NAME "+t.shaderName,g].filter(aa).join(`
`),m.length>0&&(m+=`
`),d=["#define SHADER_TYPE "+t.shaderType,"#define SHADER_NAME "+t.shaderName,g].filter(aa).join(`
`),d.length>0&&(d+=`
`)):(m=[Hp(t),"#define SHADER_TYPE "+t.shaderType,"#define SHADER_NAME "+t.shaderName,g,t.extensionClipCullDistance?"#define USE_CLIP_DISTANCE":"",t.batching?"#define USE_BATCHING":"",t.batchingColor?"#define USE_BATCHING_COLOR":"",t.instancing?"#define USE_INSTANCING":"",t.instancingColor?"#define USE_INSTANCING_COLOR":"",t.instancingMorph?"#define USE_INSTANCING_MORPH":"",t.useFog&&t.fog?"#define USE_FOG":"",t.useFog&&t.fogExp2?"#define FOG_EXP2":"",t.map?"#define USE_MAP":"",t.envMap?"#define USE_ENVMAP":"",t.envMap?"#define "+l:"",t.lightMap?"#define USE_LIGHTMAP":"",t.aoMap?"#define USE_AOMAP":"",t.bumpMap?"#define USE_BUMPMAP":"",t.normalMap?"#define USE_NORMALMAP":"",t.normalMapObjectSpace?"#define USE_NORMALMAP_OBJECTSPACE":"",t.normalMapTangentSpace?"#define USE_NORMALMAP_TANGENTSPACE":"",t.displacementMap?"#define USE_DISPLACEMENTMAP":"",t.emissiveMap?"#define USE_EMISSIVEMAP":"",t.anisotropy?"#define USE_ANISOTROPY":"",t.anisotropyMap?"#define USE_ANISOTROPYMAP":"",t.clearcoatMap?"#define USE_CLEARCOATMAP":"",t.clearcoatRoughnessMap?"#define USE_CLEARCOAT_ROUGHNESSMAP":"",t.clearcoatNormalMap?"#define USE_CLEARCOAT_NORMALMAP":"",t.iridescenceMap?"#define USE_IRIDESCENCEMAP":"",t.iridescenceThicknessMap?"#define USE_IRIDESCENCE_THICKNESSMAP":"",t.specularMap?"#define USE_SPECULARMAP":"",t.specularColorMap?"#define USE_SPECULAR_COLORMAP":"",t.specularIntensityMap?"#define USE_SPECULAR_INTENSITYMAP":"",t.roughnessMap?"#define USE_ROUGHNESSMAP":"",t.metalnessMap?"#define USE_METALNESSMAP":"",t.alphaMap?"#define USE_ALPHAMAP":"",t.alphaHash?"#define USE_ALPHAHASH":"",t.transmission?"#define USE_TRANSMISSION":"",t.transmissionMap?"#define USE_TRANSMISSIONMAP":"",t.thicknessMap?"#define USE_THICKNESSMAP":"",t.sheenColorMap?"#define USE_SHEEN_COLORMAP":"",t.sheenRoughnessMap?"#define USE_SHEEN_ROUGHNESSMAP":"",t.mapUv?"#define MAP_UV "+t.mapUv:"",t.alphaMapUv?"#define ALPHAMAP_UV "+t.alphaMapUv:"",t.lightMapUv?"#define LIGHTMAP_UV "+t.lightMapUv:"",t.aoMapUv?"#define AOMAP_UV "+t.aoMapUv:"",t.emissiveMapUv?"#define EMISSIVEMAP_UV "+t.emissiveMapUv:"",t.bumpMapUv?"#define BUMPMAP_UV "+t.bumpMapUv:"",t.normalMapUv?"#define NORMALMAP_UV "+t.normalMapUv:"",t.displacementMapUv?"#define DISPLACEMENTMAP_UV "+t.displacementMapUv:"",t.metalnessMapUv?"#define METALNESSMAP_UV "+t.metalnessMapUv:"",t.roughnessMapUv?"#define ROUGHNESSMAP_UV "+t.roughnessMapUv:"",t.anisotropyMapUv?"#define ANISOTROPYMAP_UV "+t.anisotropyMapUv:"",t.clearcoatMapUv?"#define CLEARCOATMAP_UV "+t.clearcoatMapUv:"",t.clearcoatNormalMapUv?"#define CLEARCOAT_NORMALMAP_UV "+t.clearcoatNormalMapUv:"",t.clearcoatRoughnessMapUv?"#define CLEARCOAT_ROUGHNESSMAP_UV "+t.clearcoatRoughnessMapUv:"",t.iridescenceMapUv?"#define IRIDESCENCEMAP_UV "+t.iridescenceMapUv:"",t.iridescenceThicknessMapUv?"#define IRIDESCENCE_THICKNESSMAP_UV "+t.iridescenceThicknessMapUv:"",t.sheenColorMapUv?"#define SHEEN_COLORMAP_UV "+t.sheenColorMapUv:"",t.sheenRoughnessMapUv?"#define SHEEN_ROUGHNESSMAP_UV "+t.sheenRoughnessMapUv:"",t.specularMapUv?"#define SPECULARMAP_UV "+t.specularMapUv:"",t.specularColorMapUv?"#define SPECULAR_COLORMAP_UV "+t.specularColorMapUv:"",t.specularIntensityMapUv?"#define SPECULAR_INTENSITYMAP_UV "+t.specularIntensityMapUv:"",t.transmissionMapUv?"#define TRANSMISSIONMAP_UV "+t.transmissionMapUv:"",t.thicknessMapUv?"#define THICKNESSMAP_UV "+t.thicknessMapUv:"",t.vertexTangents&&t.flatShading===!1?"#define USE_TANGENT":"",t.vertexColors?"#define USE_COLOR":"",t.vertexAlphas?"#define USE_COLOR_ALPHA":"",t.vertexUv1s?"#define USE_UV1":"",t.vertexUv2s?"#define USE_UV2":"",t.vertexUv3s?"#define USE_UV3":"",t.pointsUvs?"#define USE_POINTS_UV":"",t.flatShading?"#define FLAT_SHADED":"",t.skinning?"#define USE_SKINNING":"",t.morphTargets?"#define USE_MORPHTARGETS":"",t.morphNormals&&t.flatShading===!1?"#define USE_MORPHNORMALS":"",t.morphColors?"#define USE_MORPHCOLORS":"",t.morphTargetsCount>0?"#define MORPHTARGETS_TEXTURE_STRIDE "+t.morphTextureStride:"",t.morphTargetsCount>0?"#define MORPHTARGETS_COUNT "+t.morphTargetsCount:"",t.doubleSided?"#define DOUBLE_SIDED":"",t.flipSided?"#define FLIP_SIDED":"",t.shadowMapEnabled?"#define USE_SHADOWMAP":"",t.shadowMapEnabled?"#define "+c:"",t.sizeAttenuation?"#define USE_SIZEATTENUATION":"",t.numLightProbes>0?"#define USE_LIGHT_PROBES":"",t.logarithmicDepthBuffer?"#define USE_LOGARITHMIC_DEPTH_BUFFER":"",t.reversedDepthBuffer?"#define USE_REVERSED_DEPTH_BUFFER":"","uniform mat4 modelMatrix;","uniform mat4 modelViewMatrix;","uniform mat4 projectionMatrix;","uniform mat4 viewMatrix;","uniform mat3 normalMatrix;","uniform vec3 cameraPosition;","uniform bool isOrthographic;","#ifdef USE_INSTANCING","	attribute mat4 instanceMatrix;","#endif","#ifdef USE_INSTANCING_COLOR","	attribute vec3 instanceColor;","#endif","#ifdef USE_INSTANCING_MORPH","	uniform sampler2D morphTexture;","#endif","attribute vec3 position;","attribute vec3 normal;","attribute vec2 uv;","#ifdef USE_UV1","	attribute vec2 uv1;","#endif","#ifdef USE_UV2","	attribute vec2 uv2;","#endif","#ifdef USE_UV3","	attribute vec2 uv3;","#endif","#ifdef USE_TANGENT","	attribute vec4 tangent;","#endif","#if defined( USE_COLOR_ALPHA )","	attribute vec4 color;","#elif defined( USE_COLOR )","	attribute vec3 color;","#endif","#ifdef USE_SKINNING","	attribute vec4 skinIndex;","	attribute vec4 skinWeight;","#endif",`
`].filter(aa).join(`
`),d=[Hp(t),"#define SHADER_TYPE "+t.shaderType,"#define SHADER_NAME "+t.shaderName,g,t.useFog&&t.fog?"#define USE_FOG":"",t.useFog&&t.fogExp2?"#define FOG_EXP2":"",t.alphaToCoverage?"#define ALPHA_TO_COVERAGE":"",t.map?"#define USE_MAP":"",t.matcap?"#define USE_MATCAP":"",t.envMap?"#define USE_ENVMAP":"",t.envMap?"#define "+u:"",t.envMap?"#define "+l:"",t.envMap?"#define "+h:"",f?"#define CUBEUV_TEXEL_WIDTH "+f.texelWidth:"",f?"#define CUBEUV_TEXEL_HEIGHT "+f.texelHeight:"",f?"#define CUBEUV_MAX_MIP "+f.maxMip+".0":"",t.lightMap?"#define USE_LIGHTMAP":"",t.aoMap?"#define USE_AOMAP":"",t.bumpMap?"#define USE_BUMPMAP":"",t.normalMap?"#define USE_NORMALMAP":"",t.normalMapObjectSpace?"#define USE_NORMALMAP_OBJECTSPACE":"",t.normalMapTangentSpace?"#define USE_NORMALMAP_TANGENTSPACE":"",t.emissiveMap?"#define USE_EMISSIVEMAP":"",t.anisotropy?"#define USE_ANISOTROPY":"",t.anisotropyMap?"#define USE_ANISOTROPYMAP":"",t.clearcoat?"#define USE_CLEARCOAT":"",t.clearcoatMap?"#define USE_CLEARCOATMAP":"",t.clearcoatRoughnessMap?"#define USE_CLEARCOAT_ROUGHNESSMAP":"",t.clearcoatNormalMap?"#define USE_CLEARCOAT_NORMALMAP":"",t.dispersion?"#define USE_DISPERSION":"",t.iridescence?"#define USE_IRIDESCENCE":"",t.iridescenceMap?"#define USE_IRIDESCENCEMAP":"",t.iridescenceThicknessMap?"#define USE_IRIDESCENCE_THICKNESSMAP":"",t.specularMap?"#define USE_SPECULARMAP":"",t.specularColorMap?"#define USE_SPECULAR_COLORMAP":"",t.specularIntensityMap?"#define USE_SPECULAR_INTENSITYMAP":"",t.roughnessMap?"#define USE_ROUGHNESSMAP":"",t.metalnessMap?"#define USE_METALNESSMAP":"",t.alphaMap?"#define USE_ALPHAMAP":"",t.alphaTest?"#define USE_ALPHATEST":"",t.alphaHash?"#define USE_ALPHAHASH":"",t.sheen?"#define USE_SHEEN":"",t.sheenColorMap?"#define USE_SHEEN_COLORMAP":"",t.sheenRoughnessMap?"#define USE_SHEEN_ROUGHNESSMAP":"",t.transmission?"#define USE_TRANSMISSION":"",t.transmissionMap?"#define USE_TRANSMISSIONMAP":"",t.thicknessMap?"#define USE_THICKNESSMAP":"",t.vertexTangents&&t.flatShading===!1?"#define USE_TANGENT":"",t.vertexColors||t.instancingColor||t.batchingColor?"#define USE_COLOR":"",t.vertexAlphas?"#define USE_COLOR_ALPHA":"",t.vertexUv1s?"#define USE_UV1":"",t.vertexUv2s?"#define USE_UV2":"",t.vertexUv3s?"#define USE_UV3":"",t.pointsUvs?"#define USE_POINTS_UV":"",t.gradientMap?"#define USE_GRADIENTMAP":"",t.flatShading?"#define FLAT_SHADED":"",t.doubleSided?"#define DOUBLE_SIDED":"",t.flipSided?"#define FLIP_SIDED":"",t.shadowMapEnabled?"#define USE_SHADOWMAP":"",t.shadowMapEnabled?"#define "+c:"",t.premultipliedAlpha?"#define PREMULTIPLIED_ALPHA":"",t.numLightProbes>0?"#define USE_LIGHT_PROBES":"",t.decodeVideoTexture?"#define DECODE_VIDEO_TEXTURE":"",t.decodeVideoTextureEmissive?"#define DECODE_VIDEO_TEXTURE_EMISSIVE":"",t.logarithmicDepthBuffer?"#define USE_LOGARITHMIC_DEPTH_BUFFER":"",t.reversedDepthBuffer?"#define USE_REVERSED_DEPTH_BUFFER":"","uniform mat4 viewMatrix;","uniform vec3 cameraPosition;","uniform bool isOrthographic;",t.toneMapping!==hi?"#define TONE_MAPPING":"",t.toneMapping!==hi?He.tonemapping_pars_fragment:"",t.toneMapping!==hi?Nb("toneMapping",t.toneMapping):"",t.dithering?"#define DITHERING":"",t.opaque?"#define OPAQUE":"",He.colorspace_pars_fragment,Db("linearToOutputTexel",t.outputColorSpace),Lb(),t.useDepthPacking?"#define DEPTH_PACKING "+t.depthPacking:"",`
`].filter(aa).join(`
`)),a=Iu(a),a=zp(a,t),a=Vp(a,t),o=Iu(o),o=zp(o,t),o=Vp(o,t),a=$p(a),o=$p(o),t.isRawShaderMaterial!==!0&&(w=`#version 300 es
`,m=[p,"#define attribute in","#define varying out","#define texture2D texture"].join(`
`)+`
`+m,d=["#define varying in",t.glslVersion===mu?"":"layout(location = 0) out highp vec4 pc_fragColor;",t.glslVersion===mu?"":"#define gl_FragColor pc_fragColor","#define gl_FragDepthEXT gl_FragDepth","#define texture2D texture","#define textureCube texture","#define texture2DProj textureProj","#define texture2DLodEXT textureLod","#define texture2DProjLodEXT textureProjLod","#define textureCubeLodEXT textureLod","#define texture2DGradEXT textureGrad","#define texture2DProjGradEXT textureProjGrad","#define textureCubeGradEXT textureGrad"].join(`
`)+`
`+d);let M=w+m+a,x=w+d+o,T=Fp(r,r.VERTEX_SHADER,M),R=Fp(r,r.FRAGMENT_SHADER,x);r.attachShader(_,T),r.attachShader(_,R),t.index0AttributeName!==void 0?r.bindAttribLocation(_,0,t.index0AttributeName):t.morphTargets===!0&&r.bindAttribLocation(_,0,"position"),r.linkProgram(_);function P(I){if(n.debug.checkShaderErrors){let k=r.getProgramInfoLog(_)||"",V=r.getShaderInfoLog(T)||"",W=r.getShaderInfoLog(R)||"",j=k.trim(),G=V.trim(),ne=W.trim(),z=!0,ae=!0;if(r.getProgramParameter(_,r.LINK_STATUS)===!1)if(z=!1,typeof n.debug.onShaderError=="function")n.debug.onShaderError(r,_,T,R);else{let ue=kp(r,T,"vertex"),we=kp(r,R,"fragment");console.error("THREE.WebGLProgram: Shader Error "+r.getError()+" - VALIDATE_STATUS "+r.getProgramParameter(_,r.VALIDATE_STATUS)+`

Material Name: `+I.name+`
Material Type: `+I.type+`

Program Info Log: `+j+`
`+ue+`
`+we)}else j!==""?console.warn("THREE.WebGLProgram: Program Info Log:",j):(G===""||ne==="")&&(ae=!1);ae&&(I.diagnostics={runnable:z,programLog:j,vertexShader:{log:G,prefix:m},fragmentShader:{log:ne,prefix:d}})}r.deleteShader(T),r.deleteShader(R),O=new ss(r,_),b=Fb(r,_)}let O;this.getUniforms=function(){return O===void 0&&P(this),O};let b;this.getAttributes=function(){return b===void 0&&P(this),b};let S=t.rendererExtensionParallelShaderCompile===!1;return this.isReady=function(){return S===!1&&(S=r.getProgramParameter(_,Rb)),S},this.destroy=function(){i.releaseStatesOfProgram(this),r.deleteProgram(_),this.program=void 0},this.type=t.shaderType,this.name=t.shaderName,this.id=Cb++,this.cacheKey=e,this.usedTimes=1,this.program=_,this.vertexShader=T,this.fragmentShader=R,this}var Yb=0,Du=class{constructor(){this.shaderCache=new Map,this.materialCache=new Map}update(e){let t=e.vertexShader,i=e.fragmentShader,r=this._getShaderStage(t),s=this._getShaderStage(i),a=this._getShaderCacheForMaterial(e);return a.has(r)===!1&&(a.add(r),r.usedTimes++),a.has(s)===!1&&(a.add(s),s.usedTimes++),this}remove(e){let t=this.materialCache.get(e);for(let i of t)i.usedTimes--,i.usedTimes===0&&this.shaderCache.delete(i.code);return this.materialCache.delete(e),this}getVertexShaderID(e){return this._getShaderStage(e.vertexShader).id}getFragmentShaderID(e){return this._getShaderStage(e.fragmentShader).id}dispose(){this.shaderCache.clear(),this.materialCache.clear()}_getShaderCacheForMaterial(e){let t=this.materialCache,i=t.get(e);return i===void 0&&(i=new Set,t.set(e,i)),i}_getShaderStage(e){let t=this.shaderCache,i=t.get(e);return i===void 0&&(i=new Nu(e),t.set(e,i)),i}},Nu=class{constructor(e){this.id=Yb++,this.code=e,this.usedTimes=0}};function Zb(n,e,t,i,r,s,a){let o=new jr,c=new Du,u=new Set,l=[],h=r.logarithmicDepthBuffer,f=r.vertexTextures,p=r.precision,g={MeshDepthMaterial:"depth",MeshDistanceMaterial:"distanceRGBA",MeshNormalMaterial:"normal",MeshBasicMaterial:"basic",MeshLambertMaterial:"lambert",MeshPhongMaterial:"phong",MeshToonMaterial:"toon",MeshStandardMaterial:"physical",MeshPhysicalMaterial:"physical",MeshMatcapMaterial:"matcap",LineBasicMaterial:"basic",LineDashedMaterial:"dashed",PointsMaterial:"points",ShadowMaterial:"shadow",SpriteMaterial:"sprite"};function _(b){return u.add(b),b===0?"uv":`uv${b}`}function m(b,S,I,k,V){let W=k.fog,j=V.geometry,G=b.isMeshStandardMaterial?k.environment:null,ne=(b.isMeshStandardMaterial?t:e).get(b.envMap||G),z=ne&&ne.mapping===Qs?ne.image.height:null,ae=g[b.type];b.precision!==null&&(p=r.getMaxPrecision(b.precision),p!==b.precision&&console.warn("THREE.WebGLProgram.getParameters:",b.precision,"not supported, using",p,"instead."));let ue=j.morphAttributes.position||j.morphAttributes.normal||j.morphAttributes.color,we=ue!==void 0?ue.length:0,je=0;j.morphAttributes.position!==void 0&&(je=1),j.morphAttributes.normal!==void 0&&(je=2),j.morphAttributes.color!==void 0&&(je=3);let ft,xt,rt,q;if(ae){let st=Kn[ae];ft=st.vertexShader,xt=st.fragmentShader}else ft=b.vertexShader,xt=b.fragmentShader,c.update(b),rt=c.getVertexShaderID(b),q=c.getFragmentShaderID(b);let Z=n.getRenderTarget(),fe=n.state.buffers.depth.getReversed(),Ne=V.isInstancedMesh===!0,Ee=V.isBatchedMesh===!0,Je=!!b.map,Bt=!!b.matcap,A=!!ne,St=!!b.aoMap,Oe=!!b.lightMap,Pe=!!b.bumpMap,ge=!!b.normalMap,bt=!!b.displacementMap,_e=!!b.emissiveMap,$e=!!b.metalnessMap,Nt=!!b.roughnessMap,At=b.anisotropy>0,E=b.clearcoat>0,y=b.dispersion>0,U=b.iridescence>0,H=b.sheen>0,Y=b.transmission>0,$=At&&!!b.anisotropyMap,Me=E&&!!b.clearcoatMap,ie=E&&!!b.clearcoatNormalMap,ye=E&&!!b.clearcoatRoughnessMap,Se=U&&!!b.iridescenceMap,Q=U&&!!b.iridescenceThicknessMap,le=H&&!!b.sheenColorMap,Ce=H&&!!b.sheenRoughnessMap,be=!!b.specularMap,oe=!!b.specularColorMap,ke=!!b.specularIntensityMap,C=Y&&!!b.transmissionMap,ee=Y&&!!b.thicknessMap,re=!!b.gradientMap,de=!!b.alphaMap,K=b.alphaTest>0,X=!!b.alphaHash,me=!!b.extensions,Le=hi;b.toneMapped&&(Z===null||Z.isXRRenderTarget===!0)&&(Le=n.toneMapping);let pt={shaderID:ae,shaderType:b.type,shaderName:b.name,vertexShader:ft,fragmentShader:xt,defines:b.defines,customVertexShaderID:rt,customFragmentShaderID:q,isRawShaderMaterial:b.isRawShaderMaterial===!0,glslVersion:b.glslVersion,precision:p,batching:Ee,batchingColor:Ee&&V._colorsTexture!==null,instancing:Ne,instancingColor:Ne&&V.instanceColor!==null,instancingMorph:Ne&&V.morphTexture!==null,supportsVertexTextures:f,outputColorSpace:Z===null?n.outputColorSpace:Z.isXRRenderTarget===!0?Z.texture.colorSpace:Qi,alphaToCoverage:!!b.alphaToCoverage,map:Je,matcap:Bt,envMap:A,envMapMode:A&&ne.mapping,envMapCubeUVHeight:z,aoMap:St,lightMap:Oe,bumpMap:Pe,normalMap:ge,displacementMap:f&&bt,emissiveMap:_e,normalMapObjectSpace:ge&&b.normalMapType===hp,normalMapTangentSpace:ge&&b.normalMapType===up,metalnessMap:$e,roughnessMap:Nt,anisotropy:At,anisotropyMap:$,clearcoat:E,clearcoatMap:Me,clearcoatNormalMap:ie,clearcoatRoughnessMap:ye,dispersion:y,iridescence:U,iridescenceMap:Se,iridescenceThicknessMap:Q,sheen:H,sheenColorMap:le,sheenRoughnessMap:Ce,specularMap:be,specularColorMap:oe,specularIntensityMap:ke,transmission:Y,transmissionMap:C,thicknessMap:ee,gradientMap:re,opaque:b.transparent===!1&&b.blending===Ki&&b.alphaToCoverage===!1,alphaMap:de,alphaTest:K,alphaHash:X,combine:b.combine,mapUv:Je&&_(b.map.channel),aoMapUv:St&&_(b.aoMap.channel),lightMapUv:Oe&&_(b.lightMap.channel),bumpMapUv:Pe&&_(b.bumpMap.channel),normalMapUv:ge&&_(b.normalMap.channel),displacementMapUv:bt&&_(b.displacementMap.channel),emissiveMapUv:_e&&_(b.emissiveMap.channel),metalnessMapUv:$e&&_(b.metalnessMap.channel),roughnessMapUv:Nt&&_(b.roughnessMap.channel),anisotropyMapUv:$&&_(b.anisotropyMap.channel),clearcoatMapUv:Me&&_(b.clearcoatMap.channel),clearcoatNormalMapUv:ie&&_(b.clearcoatNormalMap.channel),clearcoatRoughnessMapUv:ye&&_(b.clearcoatRoughnessMap.channel),iridescenceMapUv:Se&&_(b.iridescenceMap.channel),iridescenceThicknessMapUv:Q&&_(b.iridescenceThicknessMap.channel),sheenColorMapUv:le&&_(b.sheenColorMap.channel),sheenRoughnessMapUv:Ce&&_(b.sheenRoughnessMap.channel),specularMapUv:be&&_(b.specularMap.channel),specularColorMapUv:oe&&_(b.specularColorMap.channel),specularIntensityMapUv:ke&&_(b.specularIntensityMap.channel),transmissionMapUv:C&&_(b.transmissionMap.channel),thicknessMapUv:ee&&_(b.thicknessMap.channel),alphaMapUv:de&&_(b.alphaMap.channel),vertexTangents:!!j.attributes.tangent&&(ge||At),vertexColors:b.vertexColors,vertexAlphas:b.vertexColors===!0&&!!j.attributes.color&&j.attributes.color.itemSize===4,pointsUvs:V.isPoints===!0&&!!j.attributes.uv&&(Je||de),fog:!!W,useFog:b.fog===!0,fogExp2:!!W&&W.isFogExp2,flatShading:b.flatShading===!0&&b.wireframe===!1,sizeAttenuation:b.sizeAttenuation===!0,logarithmicDepthBuffer:h,reversedDepthBuffer:fe,skinning:V.isSkinnedMesh===!0,morphTargets:j.morphAttributes.position!==void 0,morphNormals:j.morphAttributes.normal!==void 0,morphColors:j.morphAttributes.color!==void 0,morphTargetsCount:we,morphTextureStride:je,numDirLights:S.directional.length,numPointLights:S.point.length,numSpotLights:S.spot.length,numSpotLightMaps:S.spotLightMap.length,numRectAreaLights:S.rectArea.length,numHemiLights:S.hemi.length,numDirLightShadows:S.directionalShadowMap.length,numPointLightShadows:S.pointShadowMap.length,numSpotLightShadows:S.spotShadowMap.length,numSpotLightShadowsWithMaps:S.numSpotLightShadowsWithMaps,numLightProbes:S.numLightProbes,numClippingPlanes:a.numPlanes,numClipIntersection:a.numIntersection,dithering:b.dithering,shadowMapEnabled:n.shadowMap.enabled&&I.length>0,shadowMapType:n.shadowMap.type,toneMapping:Le,decodeVideoTexture:Je&&b.map.isVideoTexture===!0&&tt.getTransfer(b.map.colorSpace)===ot,decodeVideoTextureEmissive:_e&&b.emissiveMap.isVideoTexture===!0&&tt.getTransfer(b.emissiveMap.colorSpace)===ot,premultipliedAlpha:b.premultipliedAlpha,doubleSided:b.side===Yn,flipSided:b.side===Jt,useDepthPacking:b.depthPacking>=0,depthPacking:b.depthPacking||0,index0AttributeName:b.index0AttributeName,extensionClipCullDistance:me&&b.extensions.clipCullDistance===!0&&i.has("WEBGL_clip_cull_distance"),extensionMultiDraw:(me&&b.extensions.multiDraw===!0||Ee)&&i.has("WEBGL_multi_draw"),rendererExtensionParallelShaderCompile:i.has("KHR_parallel_shader_compile"),customProgramCacheKey:b.customProgramCacheKey()};return pt.vertexUv1s=u.has(1),pt.vertexUv2s=u.has(2),pt.vertexUv3s=u.has(3),u.clear(),pt}function d(b){let S=[];if(b.shaderID?S.push(b.shaderID):(S.push(b.customVertexShaderID),S.push(b.customFragmentShaderID)),b.defines!==void 0)for(let I in b.defines)S.push(I),S.push(b.defines[I]);return b.isRawShaderMaterial===!1&&(w(S,b),M(S,b),S.push(n.outputColorSpace)),S.push(b.customProgramCacheKey),S.join()}function w(b,S){b.push(S.precision),b.push(S.outputColorSpace),b.push(S.envMapMode),b.push(S.envMapCubeUVHeight),b.push(S.mapUv),b.push(S.alphaMapUv),b.push(S.lightMapUv),b.push(S.aoMapUv),b.push(S.bumpMapUv),b.push(S.normalMapUv),b.push(S.displacementMapUv),b.push(S.emissiveMapUv),b.push(S.metalnessMapUv),b.push(S.roughnessMapUv),b.push(S.anisotropyMapUv),b.push(S.clearcoatMapUv),b.push(S.clearcoatNormalMapUv),b.push(S.clearcoatRoughnessMapUv),b.push(S.iridescenceMapUv),b.push(S.iridescenceThicknessMapUv),b.push(S.sheenColorMapUv),b.push(S.sheenRoughnessMapUv),b.push(S.specularMapUv),b.push(S.specularColorMapUv),b.push(S.specularIntensityMapUv),b.push(S.transmissionMapUv),b.push(S.thicknessMapUv),b.push(S.combine),b.push(S.fogExp2),b.push(S.sizeAttenuation),b.push(S.morphTargetsCount),b.push(S.morphAttributeCount),b.push(S.numDirLights),b.push(S.numPointLights),b.push(S.numSpotLights),b.push(S.numSpotLightMaps),b.push(S.numHemiLights),b.push(S.numRectAreaLights),b.push(S.numDirLightShadows),b.push(S.numPointLightShadows),b.push(S.numSpotLightShadows),b.push(S.numSpotLightShadowsWithMaps),b.push(S.numLightProbes),b.push(S.shadowMapType),b.push(S.toneMapping),b.push(S.numClippingPlanes),b.push(S.numClipIntersection),b.push(S.depthPacking)}function M(b,S){o.disableAll(),S.supportsVertexTextures&&o.enable(0),S.instancing&&o.enable(1),S.instancingColor&&o.enable(2),S.instancingMorph&&o.enable(3),S.matcap&&o.enable(4),S.envMap&&o.enable(5),S.normalMapObjectSpace&&o.enable(6),S.normalMapTangentSpace&&o.enable(7),S.clearcoat&&o.enable(8),S.iridescence&&o.enable(9),S.alphaTest&&o.enable(10),S.vertexColors&&o.enable(11),S.vertexAlphas&&o.enable(12),S.vertexUv1s&&o.enable(13),S.vertexUv2s&&o.enable(14),S.vertexUv3s&&o.enable(15),S.vertexTangents&&o.enable(16),S.anisotropy&&o.enable(17),S.alphaHash&&o.enable(18),S.batching&&o.enable(19),S.dispersion&&o.enable(20),S.batchingColor&&o.enable(21),S.gradientMap&&o.enable(22),b.push(o.mask),o.disableAll(),S.fog&&o.enable(0),S.useFog&&o.enable(1),S.flatShading&&o.enable(2),S.logarithmicDepthBuffer&&o.enable(3),S.reversedDepthBuffer&&o.enable(4),S.skinning&&o.enable(5),S.morphTargets&&o.enable(6),S.morphNormals&&o.enable(7),S.morphColors&&o.enable(8),S.premultipliedAlpha&&o.enable(9),S.shadowMapEnabled&&o.enable(10),S.doubleSided&&o.enable(11),S.flipSided&&o.enable(12),S.useDepthPacking&&o.enable(13),S.dithering&&o.enable(14),S.transmission&&o.enable(15),S.sheen&&o.enable(16),S.opaque&&o.enable(17),S.pointsUvs&&o.enable(18),S.decodeVideoTexture&&o.enable(19),S.decodeVideoTextureEmissive&&o.enable(20),S.alphaToCoverage&&o.enable(21),b.push(o.mask)}function x(b){let S=g[b.type],I;if(S){let k=Kn[S];I=bp.clone(k.uniforms)}else I=b.uniforms;return I}function T(b,S){let I;for(let k=0,V=l.length;k<V;k++){let W=l[k];if(W.cacheKey===S){I=W,++I.usedTimes;break}}return I===void 0&&(I=new jb(n,S,b,s),l.push(I)),I}function R(b){if(--b.usedTimes===0){let S=l.indexOf(b);l[S]=l[l.length-1],l.pop(),b.destroy()}}function P(b){c.remove(b)}function O(){c.dispose()}return{getParameters:m,getProgramCacheKey:d,getUniforms:x,acquireProgram:T,releaseProgram:R,releaseShaderCache:P,programs:l,dispose:O}}function Kb(){let n=new WeakMap;function e(a){return n.has(a)}function t(a){let o=n.get(a);return o===void 0&&(o={},n.set(a,o)),o}function i(a){n.delete(a)}function r(a,o,c){n.get(a)[o]=c}function s(){n=new WeakMap}return{has:e,get:t,remove:i,update:r,dispose:s}}function Jb(n,e){return n.groupOrder!==e.groupOrder?n.groupOrder-e.groupOrder:n.renderOrder!==e.renderOrder?n.renderOrder-e.renderOrder:n.material.id!==e.material.id?n.material.id-e.material.id:n.z!==e.z?n.z-e.z:n.id-e.id}function Gp(n,e){return n.groupOrder!==e.groupOrder?n.groupOrder-e.groupOrder:n.renderOrder!==e.renderOrder?n.renderOrder-e.renderOrder:n.z!==e.z?e.z-n.z:n.id-e.id}function Wp(){let n=[],e=0,t=[],i=[],r=[];function s(){e=0,t.length=0,i.length=0,r.length=0}function a(h,f,p,g,_,m){let d=n[e];return d===void 0?(d={id:h.id,object:h,geometry:f,material:p,groupOrder:g,renderOrder:h.renderOrder,z:_,group:m},n[e]=d):(d.id=h.id,d.object=h,d.geometry=f,d.material=p,d.groupOrder=g,d.renderOrder=h.renderOrder,d.z=_,d.group=m),e++,d}function o(h,f,p,g,_,m){let d=a(h,f,p,g,_,m);p.transmission>0?i.push(d):p.transparent===!0?r.push(d):t.push(d)}function c(h,f,p,g,_,m){let d=a(h,f,p,g,_,m);p.transmission>0?i.unshift(d):p.transparent===!0?r.unshift(d):t.unshift(d)}function u(h,f){t.length>1&&t.sort(h||Jb),i.length>1&&i.sort(f||Gp),r.length>1&&r.sort(f||Gp)}function l(){for(let h=e,f=n.length;h<f;h++){let p=n[h];if(p.id===null)break;p.id=null,p.object=null,p.geometry=null,p.material=null,p.group=null}}return{opaque:t,transmissive:i,transparent:r,init:s,push:o,unshift:c,finish:l,sort:u}}function Qb(){let n=new WeakMap;function e(i,r){let s=n.get(i),a;return s===void 0?(a=new Wp,n.set(i,[a])):r>=s.length?(a=new Wp,s.push(a)):a=s[r],a}function t(){n=new WeakMap}return{get:e,dispose:t}}function eM(){let n={};return{get:function(e){if(n[e.id]!==void 0)return n[e.id];let t;switch(e.type){case"DirectionalLight":t={direction:new L,color:new Qe};break;case"SpotLight":t={position:new L,direction:new L,color:new Qe,distance:0,coneCos:0,penumbraCos:0,decay:0};break;case"PointLight":t={position:new L,color:new Qe,distance:0,decay:0};break;case"HemisphereLight":t={direction:new L,skyColor:new Qe,groundColor:new Qe};break;case"RectAreaLight":t={color:new Qe,position:new L,halfWidth:new L,halfHeight:new L};break}return n[e.id]=t,t}}}function tM(){let n={};return{get:function(e){if(n[e.id]!==void 0)return n[e.id];let t;switch(e.type){case"DirectionalLight":t={shadowIntensity:1,shadowBias:0,shadowNormalBias:0,shadowRadius:1,shadowMapSize:new Ue};break;case"SpotLight":t={shadowIntensity:1,shadowBias:0,shadowNormalBias:0,shadowRadius:1,shadowMapSize:new Ue};break;case"PointLight":t={shadowIntensity:1,shadowBias:0,shadowNormalBias:0,shadowRadius:1,shadowMapSize:new Ue,shadowCameraNear:1,shadowCameraFar:1e3};break}return n[e.id]=t,t}}}var nM=0;function iM(n,e){return(e.castShadow?2:0)-(n.castShadow?2:0)+(e.map?1:0)-(n.map?1:0)}function rM(n){let e=new eM,t=tM(),i={version:0,hash:{directionalLength:-1,pointLength:-1,spotLength:-1,rectAreaLength:-1,hemiLength:-1,numDirectionalShadows:-1,numPointShadows:-1,numSpotShadows:-1,numSpotMaps:-1,numLightProbes:-1},ambient:[0,0,0],probe:[],directional:[],directionalShadow:[],directionalShadowMap:[],directionalShadowMatrix:[],spot:[],spotLightMap:[],spotShadow:[],spotShadowMap:[],spotLightMatrix:[],rectArea:[],rectAreaLTC1:null,rectAreaLTC2:null,point:[],pointShadow:[],pointShadowMap:[],pointShadowMatrix:[],hemi:[],numSpotLightShadowsWithMaps:0,numLightProbes:0};for(let u=0;u<9;u++)i.probe.push(new L);let r=new L,s=new _t,a=new _t;function o(u){let l=0,h=0,f=0;for(let b=0;b<9;b++)i.probe[b].set(0,0,0);let p=0,g=0,_=0,m=0,d=0,w=0,M=0,x=0,T=0,R=0,P=0;u.sort(iM);for(let b=0,S=u.length;b<S;b++){let I=u[b],k=I.color,V=I.intensity,W=I.distance,j=I.shadow&&I.shadow.map?I.shadow.map.texture:null;if(I.isAmbientLight)l+=k.r*V,h+=k.g*V,f+=k.b*V;else if(I.isLightProbe){for(let G=0;G<9;G++)i.probe[G].addScaledVector(I.sh.coefficients[G],V);P++}else if(I.isDirectionalLight){let G=e.get(I);if(G.color.copy(I.color).multiplyScalar(I.intensity),I.castShadow){let ne=I.shadow,z=t.get(I);z.shadowIntensity=ne.intensity,z.shadowBias=ne.bias,z.shadowNormalBias=ne.normalBias,z.shadowRadius=ne.radius,z.shadowMapSize=ne.mapSize,i.directionalShadow[p]=z,i.directionalShadowMap[p]=j,i.directionalShadowMatrix[p]=I.shadow.matrix,w++}i.directional[p]=G,p++}else if(I.isSpotLight){let G=e.get(I);G.position.setFromMatrixPosition(I.matrixWorld),G.color.copy(k).multiplyScalar(V),G.distance=W,G.coneCos=Math.cos(I.angle),G.penumbraCos=Math.cos(I.angle*(1-I.penumbra)),G.decay=I.decay,i.spot[_]=G;let ne=I.shadow;if(I.map&&(i.spotLightMap[T]=I.map,T++,ne.updateMatrices(I),I.castShadow&&R++),i.spotLightMatrix[_]=ne.matrix,I.castShadow){let z=t.get(I);z.shadowIntensity=ne.intensity,z.shadowBias=ne.bias,z.shadowNormalBias=ne.normalBias,z.shadowRadius=ne.radius,z.shadowMapSize=ne.mapSize,i.spotShadow[_]=z,i.spotShadowMap[_]=j,x++}_++}else if(I.isRectAreaLight){let G=e.get(I);G.color.copy(k).multiplyScalar(V),G.halfWidth.set(I.width*.5,0,0),G.halfHeight.set(0,I.height*.5,0),i.rectArea[m]=G,m++}else if(I.isPointLight){let G=e.get(I);if(G.color.copy(I.color).multiplyScalar(I.intensity),G.distance=I.distance,G.decay=I.decay,I.castShadow){let ne=I.shadow,z=t.get(I);z.shadowIntensity=ne.intensity,z.shadowBias=ne.bias,z.shadowNormalBias=ne.normalBias,z.shadowRadius=ne.radius,z.shadowMapSize=ne.mapSize,z.shadowCameraNear=ne.camera.near,z.shadowCameraFar=ne.camera.far,i.pointShadow[g]=z,i.pointShadowMap[g]=j,i.pointShadowMatrix[g]=I.shadow.matrix,M++}i.point[g]=G,g++}else if(I.isHemisphereLight){let G=e.get(I);G.skyColor.copy(I.color).multiplyScalar(V),G.groundColor.copy(I.groundColor).multiplyScalar(V),i.hemi[d]=G,d++}}m>0&&(n.has("OES_texture_float_linear")===!0?(i.rectAreaLTC1=se.LTC_FLOAT_1,i.rectAreaLTC2=se.LTC_FLOAT_2):(i.rectAreaLTC1=se.LTC_HALF_1,i.rectAreaLTC2=se.LTC_HALF_2)),i.ambient[0]=l,i.ambient[1]=h,i.ambient[2]=f;let O=i.hash;(O.directionalLength!==p||O.pointLength!==g||O.spotLength!==_||O.rectAreaLength!==m||O.hemiLength!==d||O.numDirectionalShadows!==w||O.numPointShadows!==M||O.numSpotShadows!==x||O.numSpotMaps!==T||O.numLightProbes!==P)&&(i.directional.length=p,i.spot.length=_,i.rectArea.length=m,i.point.length=g,i.hemi.length=d,i.directionalShadow.length=w,i.directionalShadowMap.length=w,i.pointShadow.length=M,i.pointShadowMap.length=M,i.spotShadow.length=x,i.spotShadowMap.length=x,i.directionalShadowMatrix.length=w,i.pointShadowMatrix.length=M,i.spotLightMatrix.length=x+T-R,i.spotLightMap.length=T,i.numSpotLightShadowsWithMaps=R,i.numLightProbes=P,O.directionalLength=p,O.pointLength=g,O.spotLength=_,O.rectAreaLength=m,O.hemiLength=d,O.numDirectionalShadows=w,O.numPointShadows=M,O.numSpotShadows=x,O.numSpotMaps=T,O.numLightProbes=P,i.version=nM++)}function c(u,l){let h=0,f=0,p=0,g=0,_=0,m=l.matrixWorldInverse;for(let d=0,w=u.length;d<w;d++){let M=u[d];if(M.isDirectionalLight){let x=i.directional[h];x.direction.setFromMatrixPosition(M.matrixWorld),r.setFromMatrixPosition(M.target.matrixWorld),x.direction.sub(r),x.direction.transformDirection(m),h++}else if(M.isSpotLight){let x=i.spot[p];x.position.setFromMatrixPosition(M.matrixWorld),x.position.applyMatrix4(m),x.direction.setFromMatrixPosition(M.matrixWorld),r.setFromMatrixPosition(M.target.matrixWorld),x.direction.sub(r),x.direction.transformDirection(m),p++}else if(M.isRectAreaLight){let x=i.rectArea[g];x.position.setFromMatrixPosition(M.matrixWorld),x.position.applyMatrix4(m),a.identity(),s.copy(M.matrixWorld),s.premultiply(m),a.extractRotation(s),x.halfWidth.set(M.width*.5,0,0),x.halfHeight.set(0,M.height*.5,0),x.halfWidth.applyMatrix4(a),x.halfHeight.applyMatrix4(a),g++}else if(M.isPointLight){let x=i.point[f];x.position.setFromMatrixPosition(M.matrixWorld),x.position.applyMatrix4(m),f++}else if(M.isHemisphereLight){let x=i.hemi[_];x.direction.setFromMatrixPosition(M.matrixWorld),x.direction.transformDirection(m),_++}}}return{setup:o,setupView:c,state:i}}function qp(n){let e=new rM(n),t=[],i=[];function r(l){u.camera=l,t.length=0,i.length=0}function s(l){t.push(l)}function a(l){i.push(l)}function o(){e.setup(t)}function c(l){e.setupView(t,l)}let u={lightsArray:t,shadowsArray:i,camera:null,lights:e,transmissionRenderTarget:{}};return{init:r,state:u,setupLights:o,setupLightsView:c,pushLight:s,pushShadow:a}}function sM(n){let e=new WeakMap;function t(r,s=0){let a=e.get(r),o;return a===void 0?(o=new qp(n),e.set(r,[o])):s>=a.length?(o=new qp(n),a.push(o)):o=a[s],o}function i(){e=new WeakMap}return{get:t,dispose:i}}var aM=`void main() {
	gl_Position = vec4( position, 1.0 );
}`,oM=`uniform sampler2D shadow_pass;
uniform vec2 resolution;
uniform float radius;
#include <packing>
void main() {
	const float samples = float( VSM_SAMPLES );
	float mean = 0.0;
	float squared_mean = 0.0;
	float uvStride = samples <= 1.0 ? 0.0 : 2.0 / ( samples - 1.0 );
	float uvStart = samples <= 1.0 ? 0.0 : - 1.0;
	for ( float i = 0.0; i < samples; i ++ ) {
		float uvOffset = uvStart + i * uvStride;
		#ifdef HORIZONTAL_PASS
			vec2 distribution = unpackRGBATo2Half( texture2D( shadow_pass, ( gl_FragCoord.xy + vec2( uvOffset, 0.0 ) * radius ) / resolution ) );
			mean += distribution.x;
			squared_mean += distribution.y * distribution.y + distribution.x * distribution.x;
		#else
			float depth = unpackRGBAToDepth( texture2D( shadow_pass, ( gl_FragCoord.xy + vec2( 0.0, uvOffset ) * radius ) / resolution ) );
			mean += depth;
			squared_mean += depth * depth;
		#endif
	}
	mean = mean / samples;
	squared_mean = squared_mean / samples;
	float std_dev = sqrt( squared_mean - mean * mean );
	gl_FragColor = pack2HalfToRGBA( vec2( mean, std_dev ) );
}`;function cM(n,e,t){let i=new Ws,r=new Ue,s=new Ue,a=new Tt,o=new So({depthPacking:lp}),c=new bo,u={},l=t.maxTextureSize,h={[ci]:Jt,[Jt]:ci,[Yn]:Yn},f=new Fn({defines:{VSM_SAMPLES:8},uniforms:{shadow_pass:{value:null},resolution:{value:new Ue},radius:{value:4}},vertexShader:aM,fragmentShader:oM}),p=f.clone();p.defines.HORIZONTAL_PASS=1;let g=new li;g.setAttribute("position",new rn(new Float32Array([-1,-1,.5,3,-1,.5,-1,3,.5]),3));let _=new Ht(g,f),m=this;this.enabled=!1,this.autoUpdate=!0,this.needsUpdate=!1,this.type=tu;let d=this.type;this.render=function(R,P,O){if(m.enabled===!1||m.autoUpdate===!1&&m.needsUpdate===!1||R.length===0)return;let b=n.getRenderTarget(),S=n.getActiveCubeFace(),I=n.getActiveMipmapLevel(),k=n.state;k.setBlending(ui),k.buffers.depth.getReversed()===!0?k.buffers.color.setClear(0,0,0,0):k.buffers.color.setClear(1,1,1,1),k.buffers.depth.setTest(!0),k.setScissorTest(!1);let V=d!==jn&&this.type===jn,W=d===jn&&this.type!==jn;for(let j=0,G=R.length;j<G;j++){let ne=R[j],z=ne.shadow;if(z===void 0){console.warn("THREE.WebGLShadowMap:",ne,"has no shadow.");continue}if(z.autoUpdate===!1&&z.needsUpdate===!1)continue;r.copy(z.mapSize);let ae=z.getFrameExtents();if(r.multiply(ae),s.copy(z.mapSize),(r.x>l||r.y>l)&&(r.x>l&&(s.x=Math.floor(l/ae.x),r.x=s.x*ae.x,z.mapSize.x=s.x),r.y>l&&(s.y=Math.floor(l/ae.y),r.y=s.y*ae.y,z.mapSize.y=s.y)),z.map===null||V===!0||W===!0){let we=this.type!==jn?{minFilter:sn,magFilter:sn}:{};z.map!==null&&z.map.dispose(),z.map=new Wn(r.x,r.y,we),z.map.texture.name=ne.name+".shadowMap",z.camera.updateProjectionMatrix()}n.setRenderTarget(z.map),n.clear();let ue=z.getViewportCount();for(let we=0;we<ue;we++){let je=z.getViewport(we);a.set(s.x*je.x,s.y*je.y,s.x*je.z,s.y*je.w),k.viewport(a),z.updateMatrices(ne,we),i=z.getFrustum(),x(P,O,z.camera,ne,this.type)}z.isPointLightShadow!==!0&&this.type===jn&&w(z,O),z.needsUpdate=!1}d=this.type,m.needsUpdate=!1,n.setRenderTarget(b,S,I)};function w(R,P){let O=e.update(_);f.defines.VSM_SAMPLES!==R.blurSamples&&(f.defines.VSM_SAMPLES=R.blurSamples,p.defines.VSM_SAMPLES=R.blurSamples,f.needsUpdate=!0,p.needsUpdate=!0),R.mapPass===null&&(R.mapPass=new Wn(r.x,r.y)),f.uniforms.shadow_pass.value=R.map.texture,f.uniforms.resolution.value=R.mapSize,f.uniforms.radius.value=R.radius,n.setRenderTarget(R.mapPass),n.clear(),n.renderBufferDirect(P,null,O,f,_,null),p.uniforms.shadow_pass.value=R.mapPass.texture,p.uniforms.resolution.value=R.mapSize,p.uniforms.radius.value=R.radius,n.setRenderTarget(R.map),n.clear(),n.renderBufferDirect(P,null,O,p,_,null)}function M(R,P,O,b){let S=null,I=O.isPointLight===!0?R.customDistanceMaterial:R.customDepthMaterial;if(I!==void 0)S=I;else if(S=O.isPointLight===!0?c:o,n.localClippingEnabled&&P.clipShadows===!0&&Array.isArray(P.clippingPlanes)&&P.clippingPlanes.length!==0||P.displacementMap&&P.displacementScale!==0||P.alphaMap&&P.alphaTest>0||P.map&&P.alphaTest>0||P.alphaToCoverage===!0){let k=S.uuid,V=P.uuid,W=u[k];W===void 0&&(W={},u[k]=W);let j=W[V];j===void 0&&(j=S.clone(),W[V]=j,P.addEventListener("dispose",T)),S=j}if(S.visible=P.visible,S.wireframe=P.wireframe,b===jn?S.side=P.shadowSide!==null?P.shadowSide:P.side:S.side=P.shadowSide!==null?P.shadowSide:h[P.side],S.alphaMap=P.alphaMap,S.alphaTest=P.alphaToCoverage===!0?.5:P.alphaTest,S.map=P.map,S.clipShadows=P.clipShadows,S.clippingPlanes=P.clippingPlanes,S.clipIntersection=P.clipIntersection,S.displacementMap=P.displacementMap,S.displacementScale=P.displacementScale,S.displacementBias=P.displacementBias,S.wireframeLinewidth=P.wireframeLinewidth,S.linewidth=P.linewidth,O.isPointLight===!0&&S.isMeshDistanceMaterial===!0){let k=n.properties.get(S);k.light=O}return S}function x(R,P,O,b,S){if(R.visible===!1)return;if(R.layers.test(P.layers)&&(R.isMesh||R.isLine||R.isPoints)&&(R.castShadow||R.receiveShadow&&S===jn)&&(!R.frustumCulled||i.intersectsObject(R))){R.modelViewMatrix.multiplyMatrices(O.matrixWorldInverse,R.matrixWorld);let V=e.update(R),W=R.material;if(Array.isArray(W)){let j=V.groups;for(let G=0,ne=j.length;G<ne;G++){let z=j[G],ae=W[z.materialIndex];if(ae&&ae.visible){let ue=M(R,ae,b,S);R.onBeforeShadow(n,R,P,O,V,ue,z),n.renderBufferDirect(O,null,V,ue,R,z),R.onAfterShadow(n,R,P,O,V,ue,z)}}}else if(W.visible){let j=M(R,W,b,S);R.onBeforeShadow(n,R,P,O,V,j,null),n.renderBufferDirect(O,null,V,j,R,null),R.onAfterShadow(n,R,P,O,V,j,null)}}let k=R.children;for(let V=0,W=k.length;V<W;V++)x(k[V],P,O,b,S)}function T(R){R.target.removeEventListener("dispose",T);for(let O in u){let b=u[O],S=R.target.uuid;S in b&&(b[S].dispose(),delete b[S])}}}var lM={[Lo]:Uo,[Oo]:ko,[Fo]:zo,[Ji]:Bo,[Uo]:Lo,[ko]:Oo,[zo]:Fo,[Bo]:Ji};function uM(n,e){function t(){let C=!1,ee=new Tt,re=null,de=new Tt(0,0,0,0);return{setMask:function(K){re!==K&&!C&&(n.colorMask(K,K,K,K),re=K)},setLocked:function(K){C=K},setClear:function(K,X,me,Le,pt){pt===!0&&(K*=Le,X*=Le,me*=Le),ee.set(K,X,me,Le),de.equals(ee)===!1&&(n.clearColor(K,X,me,Le),de.copy(ee))},reset:function(){C=!1,re=null,de.set(-1,0,0,0)}}}function i(){let C=!1,ee=!1,re=null,de=null,K=null;return{setReversed:function(X){if(ee!==X){let me=e.get("EXT_clip_control");X?me.clipControlEXT(me.LOWER_LEFT_EXT,me.ZERO_TO_ONE_EXT):me.clipControlEXT(me.LOWER_LEFT_EXT,me.NEGATIVE_ONE_TO_ONE_EXT),ee=X;let Le=K;K=null,this.setClear(Le)}},getReversed:function(){return ee},setTest:function(X){X?Z(n.DEPTH_TEST):fe(n.DEPTH_TEST)},setMask:function(X){re!==X&&!C&&(n.depthMask(X),re=X)},setFunc:function(X){if(ee&&(X=lM[X]),de!==X){switch(X){case Lo:n.depthFunc(n.NEVER);break;case Uo:n.depthFunc(n.ALWAYS);break;case Oo:n.depthFunc(n.LESS);break;case Ji:n.depthFunc(n.LEQUAL);break;case Fo:n.depthFunc(n.EQUAL);break;case Bo:n.depthFunc(n.GEQUAL);break;case ko:n.depthFunc(n.GREATER);break;case zo:n.depthFunc(n.NOTEQUAL);break;default:n.depthFunc(n.LEQUAL)}de=X}},setLocked:function(X){C=X},setClear:function(X){K!==X&&(ee&&(X=1-X),n.clearDepth(X),K=X)},reset:function(){C=!1,re=null,de=null,K=null,ee=!1}}}function r(){let C=!1,ee=null,re=null,de=null,K=null,X=null,me=null,Le=null,pt=null;return{setTest:function(st){C||(st?Z(n.STENCIL_TEST):fe(n.STENCIL_TEST))},setMask:function(st){ee!==st&&!C&&(n.stencilMask(st),ee=st)},setFunc:function(st,ti,Hn){(re!==st||de!==ti||K!==Hn)&&(n.stencilFunc(st,ti,Hn),re=st,de=ti,K=Hn)},setOp:function(st,ti,Hn){(X!==st||me!==ti||Le!==Hn)&&(n.stencilOp(st,ti,Hn),X=st,me=ti,Le=Hn)},setLocked:function(st){C=st},setClear:function(st){pt!==st&&(n.clearStencil(st),pt=st)},reset:function(){C=!1,ee=null,re=null,de=null,K=null,X=null,me=null,Le=null,pt=null}}}let s=new t,a=new i,o=new r,c=new WeakMap,u=new WeakMap,l={},h={},f=new WeakMap,p=[],g=null,_=!1,m=null,d=null,w=null,M=null,x=null,T=null,R=null,P=new Qe(0,0,0),O=0,b=!1,S=null,I=null,k=null,V=null,W=null,j=n.getParameter(n.MAX_COMBINED_TEXTURE_IMAGE_UNITS),G=!1,ne=0,z=n.getParameter(n.VERSION);z.indexOf("WebGL")!==-1?(ne=parseFloat(/^WebGL (\d)/.exec(z)[1]),G=ne>=1):z.indexOf("OpenGL ES")!==-1&&(ne=parseFloat(/^OpenGL ES (\d)/.exec(z)[1]),G=ne>=2);let ae=null,ue={},we=n.getParameter(n.SCISSOR_BOX),je=n.getParameter(n.VIEWPORT),ft=new Tt().fromArray(we),xt=new Tt().fromArray(je);function rt(C,ee,re,de){let K=new Uint8Array(4),X=n.createTexture();n.bindTexture(C,X),n.texParameteri(C,n.TEXTURE_MIN_FILTER,n.NEAREST),n.texParameteri(C,n.TEXTURE_MAG_FILTER,n.NEAREST);for(let me=0;me<re;me++)C===n.TEXTURE_3D||C===n.TEXTURE_2D_ARRAY?n.texImage3D(ee,0,n.RGBA,1,1,de,0,n.RGBA,n.UNSIGNED_BYTE,K):n.texImage2D(ee+me,0,n.RGBA,1,1,0,n.RGBA,n.UNSIGNED_BYTE,K);return X}let q={};q[n.TEXTURE_2D]=rt(n.TEXTURE_2D,n.TEXTURE_2D,1),q[n.TEXTURE_CUBE_MAP]=rt(n.TEXTURE_CUBE_MAP,n.TEXTURE_CUBE_MAP_POSITIVE_X,6),q[n.TEXTURE_2D_ARRAY]=rt(n.TEXTURE_2D_ARRAY,n.TEXTURE_2D_ARRAY,1,1),q[n.TEXTURE_3D]=rt(n.TEXTURE_3D,n.TEXTURE_3D,1,1),s.setClear(0,0,0,1),a.setClear(1),o.setClear(0),Z(n.DEPTH_TEST),a.setFunc(Ji),Pe(!1),ge(eu),Z(n.CULL_FACE),St(ui);function Z(C){l[C]!==!0&&(n.enable(C),l[C]=!0)}function fe(C){l[C]!==!1&&(n.disable(C),l[C]=!1)}function Ne(C,ee){return h[C]!==ee?(n.bindFramebuffer(C,ee),h[C]=ee,C===n.DRAW_FRAMEBUFFER&&(h[n.FRAMEBUFFER]=ee),C===n.FRAMEBUFFER&&(h[n.DRAW_FRAMEBUFFER]=ee),!0):!1}function Ee(C,ee){let re=p,de=!1;if(C){re=f.get(ee),re===void 0&&(re=[],f.set(ee,re));let K=C.textures;if(re.length!==K.length||re[0]!==n.COLOR_ATTACHMENT0){for(let X=0,me=K.length;X<me;X++)re[X]=n.COLOR_ATTACHMENT0+X;re.length=K.length,de=!0}}else re[0]!==n.BACK&&(re[0]=n.BACK,de=!0);de&&n.drawBuffers(re)}function Je(C){return g!==C?(n.useProgram(C),g=C,!0):!1}let Bt={[wi]:n.FUNC_ADD,[Uf]:n.FUNC_SUBTRACT,[Of]:n.FUNC_REVERSE_SUBTRACT};Bt[Ff]=n.MIN,Bt[Bf]=n.MAX;let A={[kf]:n.ZERO,[zf]:n.ONE,[Vf]:n.SRC_COLOR,[lo]:n.SRC_ALPHA,[Xf]:n.SRC_ALPHA_SATURATE,[Wf]:n.DST_COLOR,[Hf]:n.DST_ALPHA,[$f]:n.ONE_MINUS_SRC_COLOR,[uo]:n.ONE_MINUS_SRC_ALPHA,[qf]:n.ONE_MINUS_DST_COLOR,[Gf]:n.ONE_MINUS_DST_ALPHA,[jf]:n.CONSTANT_COLOR,[Yf]:n.ONE_MINUS_CONSTANT_COLOR,[Zf]:n.CONSTANT_ALPHA,[Kf]:n.ONE_MINUS_CONSTANT_ALPHA};function St(C,ee,re,de,K,X,me,Le,pt,st){if(C===ui){_===!0&&(fe(n.BLEND),_=!1);return}if(_===!1&&(Z(n.BLEND),_=!0),C!==Lf){if(C!==m||st!==b){if((d!==wi||x!==wi)&&(n.blendEquation(n.FUNC_ADD),d=wi,x=wi),st)switch(C){case Ki:n.blendFuncSeparate(n.ONE,n.ONE_MINUS_SRC_ALPHA,n.ONE,n.ONE_MINUS_SRC_ALPHA);break;case nu:n.blendFunc(n.ONE,n.ONE);break;case iu:n.blendFuncSeparate(n.ZERO,n.ONE_MINUS_SRC_COLOR,n.ZERO,n.ONE);break;case ru:n.blendFuncSeparate(n.DST_COLOR,n.ONE_MINUS_SRC_ALPHA,n.ZERO,n.ONE);break;default:console.error("THREE.WebGLState: Invalid blending: ",C);break}else switch(C){case Ki:n.blendFuncSeparate(n.SRC_ALPHA,n.ONE_MINUS_SRC_ALPHA,n.ONE,n.ONE_MINUS_SRC_ALPHA);break;case nu:n.blendFuncSeparate(n.SRC_ALPHA,n.ONE,n.ONE,n.ONE);break;case iu:console.error("THREE.WebGLState: SubtractiveBlending requires material.premultipliedAlpha = true");break;case ru:console.error("THREE.WebGLState: MultiplyBlending requires material.premultipliedAlpha = true");break;default:console.error("THREE.WebGLState: Invalid blending: ",C);break}w=null,M=null,T=null,R=null,P.set(0,0,0),O=0,m=C,b=st}return}K=K||ee,X=X||re,me=me||de,(ee!==d||K!==x)&&(n.blendEquationSeparate(Bt[ee],Bt[K]),d=ee,x=K),(re!==w||de!==M||X!==T||me!==R)&&(n.blendFuncSeparate(A[re],A[de],A[X],A[me]),w=re,M=de,T=X,R=me),(Le.equals(P)===!1||pt!==O)&&(n.blendColor(Le.r,Le.g,Le.b,pt),P.copy(Le),O=pt),m=C,b=!1}function Oe(C,ee){C.side===Yn?fe(n.CULL_FACE):Z(n.CULL_FACE);let re=C.side===Jt;ee&&(re=!re),Pe(re),C.blending===Ki&&C.transparent===!1?St(ui):St(C.blending,C.blendEquation,C.blendSrc,C.blendDst,C.blendEquationAlpha,C.blendSrcAlpha,C.blendDstAlpha,C.blendColor,C.blendAlpha,C.premultipliedAlpha),a.setFunc(C.depthFunc),a.setTest(C.depthTest),a.setMask(C.depthWrite),s.setMask(C.colorWrite);let de=C.stencilWrite;o.setTest(de),de&&(o.setMask(C.stencilWriteMask),o.setFunc(C.stencilFunc,C.stencilRef,C.stencilFuncMask),o.setOp(C.stencilFail,C.stencilZFail,C.stencilZPass)),_e(C.polygonOffset,C.polygonOffsetFactor,C.polygonOffsetUnits),C.alphaToCoverage===!0?Z(n.SAMPLE_ALPHA_TO_COVERAGE):fe(n.SAMPLE_ALPHA_TO_COVERAGE)}function Pe(C){S!==C&&(C?n.frontFace(n.CW):n.frontFace(n.CCW),S=C)}function ge(C){C!==If?(Z(n.CULL_FACE),C!==I&&(C===eu?n.cullFace(n.BACK):C===Df?n.cullFace(n.FRONT):n.cullFace(n.FRONT_AND_BACK))):fe(n.CULL_FACE),I=C}function bt(C){C!==k&&(G&&n.lineWidth(C),k=C)}function _e(C,ee,re){C?(Z(n.POLYGON_OFFSET_FILL),(V!==ee||W!==re)&&(n.polygonOffset(ee,re),V=ee,W=re)):fe(n.POLYGON_OFFSET_FILL)}function $e(C){C?Z(n.SCISSOR_TEST):fe(n.SCISSOR_TEST)}function Nt(C){C===void 0&&(C=n.TEXTURE0+j-1),ae!==C&&(n.activeTexture(C),ae=C)}function At(C,ee,re){re===void 0&&(ae===null?re=n.TEXTURE0+j-1:re=ae);let de=ue[re];de===void 0&&(de={type:void 0,texture:void 0},ue[re]=de),(de.type!==C||de.texture!==ee)&&(ae!==re&&(n.activeTexture(re),ae=re),n.bindTexture(C,ee||q[C]),de.type=C,de.texture=ee)}function E(){let C=ue[ae];C!==void 0&&C.type!==void 0&&(n.bindTexture(C.type,null),C.type=void 0,C.texture=void 0)}function y(){try{n.compressedTexImage2D(...arguments)}catch(C){console.error("THREE.WebGLState:",C)}}function U(){try{n.compressedTexImage3D(...arguments)}catch(C){console.error("THREE.WebGLState:",C)}}function H(){try{n.texSubImage2D(...arguments)}catch(C){console.error("THREE.WebGLState:",C)}}function Y(){try{n.texSubImage3D(...arguments)}catch(C){console.error("THREE.WebGLState:",C)}}function $(){try{n.compressedTexSubImage2D(...arguments)}catch(C){console.error("THREE.WebGLState:",C)}}function Me(){try{n.compressedTexSubImage3D(...arguments)}catch(C){console.error("THREE.WebGLState:",C)}}function ie(){try{n.texStorage2D(...arguments)}catch(C){console.error("THREE.WebGLState:",C)}}function ye(){try{n.texStorage3D(...arguments)}catch(C){console.error("THREE.WebGLState:",C)}}function Se(){try{n.texImage2D(...arguments)}catch(C){console.error("THREE.WebGLState:",C)}}function Q(){try{n.texImage3D(...arguments)}catch(C){console.error("THREE.WebGLState:",C)}}function le(C){ft.equals(C)===!1&&(n.scissor(C.x,C.y,C.z,C.w),ft.copy(C))}function Ce(C){xt.equals(C)===!1&&(n.viewport(C.x,C.y,C.z,C.w),xt.copy(C))}function be(C,ee){let re=u.get(ee);re===void 0&&(re=new WeakMap,u.set(ee,re));let de=re.get(C);de===void 0&&(de=n.getUniformBlockIndex(ee,C.name),re.set(C,de))}function oe(C,ee){let de=u.get(ee).get(C);c.get(ee)!==de&&(n.uniformBlockBinding(ee,de,C.__bindingPointIndex),c.set(ee,de))}function ke(){n.disable(n.BLEND),n.disable(n.CULL_FACE),n.disable(n.DEPTH_TEST),n.disable(n.POLYGON_OFFSET_FILL),n.disable(n.SCISSOR_TEST),n.disable(n.STENCIL_TEST),n.disable(n.SAMPLE_ALPHA_TO_COVERAGE),n.blendEquation(n.FUNC_ADD),n.blendFunc(n.ONE,n.ZERO),n.blendFuncSeparate(n.ONE,n.ZERO,n.ONE,n.ZERO),n.blendColor(0,0,0,0),n.colorMask(!0,!0,!0,!0),n.clearColor(0,0,0,0),n.depthMask(!0),n.depthFunc(n.LESS),a.setReversed(!1),n.clearDepth(1),n.stencilMask(4294967295),n.stencilFunc(n.ALWAYS,0,4294967295),n.stencilOp(n.KEEP,n.KEEP,n.KEEP),n.clearStencil(0),n.cullFace(n.BACK),n.frontFace(n.CCW),n.polygonOffset(0,0),n.activeTexture(n.TEXTURE0),n.bindFramebuffer(n.FRAMEBUFFER,null),n.bindFramebuffer(n.DRAW_FRAMEBUFFER,null),n.bindFramebuffer(n.READ_FRAMEBUFFER,null),n.useProgram(null),n.lineWidth(1),n.scissor(0,0,n.canvas.width,n.canvas.height),n.viewport(0,0,n.canvas.width,n.canvas.height),l={},ae=null,ue={},h={},f=new WeakMap,p=[],g=null,_=!1,m=null,d=null,w=null,M=null,x=null,T=null,R=null,P=new Qe(0,0,0),O=0,b=!1,S=null,I=null,k=null,V=null,W=null,ft.set(0,0,n.canvas.width,n.canvas.height),xt.set(0,0,n.canvas.width,n.canvas.height),s.reset(),a.reset(),o.reset()}return{buffers:{color:s,depth:a,stencil:o},enable:Z,disable:fe,bindFramebuffer:Ne,drawBuffers:Ee,useProgram:Je,setBlending:St,setMaterial:Oe,setFlipSided:Pe,setCullFace:ge,setLineWidth:bt,setPolygonOffset:_e,setScissorTest:$e,activeTexture:Nt,bindTexture:At,unbindTexture:E,compressedTexImage2D:y,compressedTexImage3D:U,texImage2D:Se,texImage3D:Q,updateUBOMapping:be,uniformBlockBinding:oe,texStorage2D:ie,texStorage3D:ye,texSubImage2D:H,texSubImage3D:Y,compressedTexSubImage2D:$,compressedTexSubImage3D:Me,scissor:le,viewport:Ce,reset:ke}}function hM(n,e,t,i,r,s,a){let o=e.has("WEBGL_multisampled_render_to_texture")?e.get("WEBGL_multisampled_render_to_texture"):null,c=typeof navigator>"u"?!1:/OculusBrowser/g.test(navigator.userAgent),u=new Ue,l=new WeakMap,h,f=new WeakMap,p=!1;try{p=typeof OffscreenCanvas<"u"&&new OffscreenCanvas(1,1).getContext("2d")!==null}catch{}function g(E,y){return p?new OffscreenCanvas(E,y):Os("canvas")}function _(E,y,U){let H=1,Y=At(E);if((Y.width>U||Y.height>U)&&(H=U/Math.max(Y.width,Y.height)),H<1)if(typeof HTMLImageElement<"u"&&E instanceof HTMLImageElement||typeof HTMLCanvasElement<"u"&&E instanceof HTMLCanvasElement||typeof ImageBitmap<"u"&&E instanceof ImageBitmap||typeof VideoFrame<"u"&&E instanceof VideoFrame){let $=Math.floor(H*Y.width),Me=Math.floor(H*Y.height);h===void 0&&(h=g($,Me));let ie=y?g($,Me):h;return ie.width=$,ie.height=Me,ie.getContext("2d").drawImage(E,0,0,$,Me),console.warn("THREE.WebGLRenderer: Texture has been resized from ("+Y.width+"x"+Y.height+") to ("+$+"x"+Me+")."),ie}else return"data"in E&&console.warn("THREE.WebGLRenderer: Image in DataTexture is too big ("+Y.width+"x"+Y.height+")."),E;return E}function m(E){return E.generateMipmaps}function d(E){n.generateMipmap(E)}function w(E){return E.isWebGLCubeRenderTarget?n.TEXTURE_CUBE_MAP:E.isWebGL3DRenderTarget?n.TEXTURE_3D:E.isWebGLArrayRenderTarget||E.isCompressedArrayTexture?n.TEXTURE_2D_ARRAY:n.TEXTURE_2D}function M(E,y,U,H,Y=!1){if(E!==null){if(n[E]!==void 0)return n[E];console.warn("THREE.WebGLRenderer: Attempt to use non-existing WebGL internal format '"+E+"'")}let $=y;if(y===n.RED&&(U===n.FLOAT&&($=n.R32F),U===n.HALF_FLOAT&&($=n.R16F),U===n.UNSIGNED_BYTE&&($=n.R8)),y===n.RED_INTEGER&&(U===n.UNSIGNED_BYTE&&($=n.R8UI),U===n.UNSIGNED_SHORT&&($=n.R16UI),U===n.UNSIGNED_INT&&($=n.R32UI),U===n.BYTE&&($=n.R8I),U===n.SHORT&&($=n.R16I),U===n.INT&&($=n.R32I)),y===n.RG&&(U===n.FLOAT&&($=n.RG32F),U===n.HALF_FLOAT&&($=n.RG16F),U===n.UNSIGNED_BYTE&&($=n.RG8)),y===n.RG_INTEGER&&(U===n.UNSIGNED_BYTE&&($=n.RG8UI),U===n.UNSIGNED_SHORT&&($=n.RG16UI),U===n.UNSIGNED_INT&&($=n.RG32UI),U===n.BYTE&&($=n.RG8I),U===n.SHORT&&($=n.RG16I),U===n.INT&&($=n.RG32I)),y===n.RGB_INTEGER&&(U===n.UNSIGNED_BYTE&&($=n.RGB8UI),U===n.UNSIGNED_SHORT&&($=n.RGB16UI),U===n.UNSIGNED_INT&&($=n.RGB32UI),U===n.BYTE&&($=n.RGB8I),U===n.SHORT&&($=n.RGB16I),U===n.INT&&($=n.RGB32I)),y===n.RGBA_INTEGER&&(U===n.UNSIGNED_BYTE&&($=n.RGBA8UI),U===n.UNSIGNED_SHORT&&($=n.RGBA16UI),U===n.UNSIGNED_INT&&($=n.RGBA32UI),U===n.BYTE&&($=n.RGBA8I),U===n.SHORT&&($=n.RGBA16I),U===n.INT&&($=n.RGBA32I)),y===n.RGB&&(U===n.UNSIGNED_INT_5_9_9_9_REV&&($=n.RGB9_E5),U===n.UNSIGNED_INT_10F_11F_11F_REV&&($=n.R11F_G11F_B10F)),y===n.RGBA){let Me=Y?Ls:tt.getTransfer(H);U===n.FLOAT&&($=n.RGBA32F),U===n.HALF_FLOAT&&($=n.RGBA16F),U===n.UNSIGNED_BYTE&&($=Me===ot?n.SRGB8_ALPHA8:n.RGBA8),U===n.UNSIGNED_SHORT_4_4_4_4&&($=n.RGBA4),U===n.UNSIGNED_SHORT_5_5_5_1&&($=n.RGB5_A1)}return($===n.R16F||$===n.R32F||$===n.RG16F||$===n.RG32F||$===n.RGBA16F||$===n.RGBA32F)&&e.get("EXT_color_buffer_float"),$}function x(E,y){let U;return E?y===null||y===Ni||y===es?U=n.DEPTH24_STENCIL8:y===Bn?U=n.DEPTH32F_STENCIL8:y===Jr&&(U=n.DEPTH24_STENCIL8,console.warn("DepthTexture: 16 bit depth attachment is not supported with stencil. Using 24-bit attachment.")):y===null||y===Ni||y===es?U=n.DEPTH_COMPONENT24:y===Bn?U=n.DEPTH_COMPONENT32F:y===Jr&&(U=n.DEPTH_COMPONENT16),U}function T(E,y){return m(E)===!0||E.isFramebufferTexture&&E.minFilter!==sn&&E.minFilter!==On?Math.log2(Math.max(y.width,y.height))+1:E.mipmaps!==void 0&&E.mipmaps.length>0?E.mipmaps.length:E.isCompressedTexture&&Array.isArray(E.image)?y.mipmaps.length:1}function R(E){let y=E.target;y.removeEventListener("dispose",R),O(y),y.isVideoTexture&&l.delete(y)}function P(E){let y=E.target;y.removeEventListener("dispose",P),S(y)}function O(E){let y=i.get(E);if(y.__webglInit===void 0)return;let U=E.source,H=f.get(U);if(H){let Y=H[y.__cacheKey];Y.usedTimes--,Y.usedTimes===0&&b(E),Object.keys(H).length===0&&f.delete(U)}i.remove(E)}function b(E){let y=i.get(E);n.deleteTexture(y.__webglTexture);let U=E.source,H=f.get(U);delete H[y.__cacheKey],a.memory.textures--}function S(E){let y=i.get(E);if(E.depthTexture&&(E.depthTexture.dispose(),i.remove(E.depthTexture)),E.isWebGLCubeRenderTarget)for(let H=0;H<6;H++){if(Array.isArray(y.__webglFramebuffer[H]))for(let Y=0;Y<y.__webglFramebuffer[H].length;Y++)n.deleteFramebuffer(y.__webglFramebuffer[H][Y]);else n.deleteFramebuffer(y.__webglFramebuffer[H]);y.__webglDepthbuffer&&n.deleteRenderbuffer(y.__webglDepthbuffer[H])}else{if(Array.isArray(y.__webglFramebuffer))for(let H=0;H<y.__webglFramebuffer.length;H++)n.deleteFramebuffer(y.__webglFramebuffer[H]);else n.deleteFramebuffer(y.__webglFramebuffer);if(y.__webglDepthbuffer&&n.deleteRenderbuffer(y.__webglDepthbuffer),y.__webglMultisampledFramebuffer&&n.deleteFramebuffer(y.__webglMultisampledFramebuffer),y.__webglColorRenderbuffer)for(let H=0;H<y.__webglColorRenderbuffer.length;H++)y.__webglColorRenderbuffer[H]&&n.deleteRenderbuffer(y.__webglColorRenderbuffer[H]);y.__webglDepthRenderbuffer&&n.deleteRenderbuffer(y.__webglDepthRenderbuffer)}let U=E.textures;for(let H=0,Y=U.length;H<Y;H++){let $=i.get(U[H]);$.__webglTexture&&(n.deleteTexture($.__webglTexture),a.memory.textures--),i.remove(U[H])}i.remove(E)}let I=0;function k(){I=0}function V(){let E=I;return E>=r.maxTextures&&console.warn("THREE.WebGLTextures: Trying to use "+E+" texture units while this GPU supports only "+r.maxTextures),I+=1,E}function W(E){let y=[];return y.push(E.wrapS),y.push(E.wrapT),y.push(E.wrapR||0),y.push(E.magFilter),y.push(E.minFilter),y.push(E.anisotropy),y.push(E.internalFormat),y.push(E.format),y.push(E.type),y.push(E.generateMipmaps),y.push(E.premultiplyAlpha),y.push(E.flipY),y.push(E.unpackAlignment),y.push(E.colorSpace),y.join()}function j(E,y){let U=i.get(E);if(E.isVideoTexture&&$e(E),E.isRenderTargetTexture===!1&&E.isExternalTexture!==!0&&E.version>0&&U.__version!==E.version){let H=E.image;if(H===null)console.warn("THREE.WebGLRenderer: Texture marked for update but no image data found.");else if(H.complete===!1)console.warn("THREE.WebGLRenderer: Texture marked for update but image is incomplete");else{q(U,E,y);return}}else E.isExternalTexture&&(U.__webglTexture=E.sourceTexture?E.sourceTexture:null);t.bindTexture(n.TEXTURE_2D,U.__webglTexture,n.TEXTURE0+y)}function G(E,y){let U=i.get(E);if(E.isRenderTargetTexture===!1&&E.version>0&&U.__version!==E.version){q(U,E,y);return}t.bindTexture(n.TEXTURE_2D_ARRAY,U.__webglTexture,n.TEXTURE0+y)}function ne(E,y){let U=i.get(E);if(E.isRenderTargetTexture===!1&&E.version>0&&U.__version!==E.version){q(U,E,y);return}t.bindTexture(n.TEXTURE_3D,U.__webglTexture,n.TEXTURE0+y)}function z(E,y){let U=i.get(E);if(E.version>0&&U.__version!==E.version){Z(U,E,y);return}t.bindTexture(n.TEXTURE_CUBE_MAP,U.__webglTexture,n.TEXTURE0+y)}let ae={[ho]:n.REPEAT,[Ei]:n.CLAMP_TO_EDGE,[fo]:n.MIRRORED_REPEAT},ue={[sn]:n.NEAREST,[op]:n.NEAREST_MIPMAP_NEAREST,[ea]:n.NEAREST_MIPMAP_LINEAR,[On]:n.LINEAR,[Ho]:n.LINEAR_MIPMAP_NEAREST,[Di]:n.LINEAR_MIPMAP_LINEAR},we={[dp]:n.NEVER,[yp]:n.ALWAYS,[fp]:n.LESS,[pu]:n.LEQUAL,[pp]:n.EQUAL,[_p]:n.GEQUAL,[mp]:n.GREATER,[gp]:n.NOTEQUAL};function je(E,y){if(y.type===Bn&&e.has("OES_texture_float_linear")===!1&&(y.magFilter===On||y.magFilter===Ho||y.magFilter===ea||y.magFilter===Di||y.minFilter===On||y.minFilter===Ho||y.minFilter===ea||y.minFilter===Di)&&console.warn("THREE.WebGLRenderer: Unable to use linear filtering with floating point textures. OES_texture_float_linear not supported on this device."),n.texParameteri(E,n.TEXTURE_WRAP_S,ae[y.wrapS]),n.texParameteri(E,n.TEXTURE_WRAP_T,ae[y.wrapT]),(E===n.TEXTURE_3D||E===n.TEXTURE_2D_ARRAY)&&n.texParameteri(E,n.TEXTURE_WRAP_R,ae[y.wrapR]),n.texParameteri(E,n.TEXTURE_MAG_FILTER,ue[y.magFilter]),n.texParameteri(E,n.TEXTURE_MIN_FILTER,ue[y.minFilter]),y.compareFunction&&(n.texParameteri(E,n.TEXTURE_COMPARE_MODE,n.COMPARE_REF_TO_TEXTURE),n.texParameteri(E,n.TEXTURE_COMPARE_FUNC,we[y.compareFunction])),e.has("EXT_texture_filter_anisotropic")===!0){if(y.magFilter===sn||y.minFilter!==ea&&y.minFilter!==Di||y.type===Bn&&e.has("OES_texture_float_linear")===!1)return;if(y.anisotropy>1||i.get(y).__currentAnisotropy){let U=e.get("EXT_texture_filter_anisotropic");n.texParameterf(E,U.TEXTURE_MAX_ANISOTROPY_EXT,Math.min(y.anisotropy,r.getMaxAnisotropy())),i.get(y).__currentAnisotropy=y.anisotropy}}}function ft(E,y){let U=!1;E.__webglInit===void 0&&(E.__webglInit=!0,y.addEventListener("dispose",R));let H=y.source,Y=f.get(H);Y===void 0&&(Y={},f.set(H,Y));let $=W(y);if($!==E.__cacheKey){Y[$]===void 0&&(Y[$]={texture:n.createTexture(),usedTimes:0},a.memory.textures++,U=!0),Y[$].usedTimes++;let Me=Y[E.__cacheKey];Me!==void 0&&(Y[E.__cacheKey].usedTimes--,Me.usedTimes===0&&b(y)),E.__cacheKey=$,E.__webglTexture=Y[$].texture}return U}function xt(E,y,U){return Math.floor(Math.floor(E/U)/y)}function rt(E,y,U,H){let $=E.updateRanges;if($.length===0)t.texSubImage2D(n.TEXTURE_2D,0,0,0,y.width,y.height,U,H,y.data);else{$.sort((Q,le)=>Q.start-le.start);let Me=0;for(let Q=1;Q<$.length;Q++){let le=$[Me],Ce=$[Q],be=le.start+le.count,oe=xt(Ce.start,y.width,4),ke=xt(le.start,y.width,4);Ce.start<=be+1&&oe===ke&&xt(Ce.start+Ce.count-1,y.width,4)===oe?le.count=Math.max(le.count,Ce.start+Ce.count-le.start):(++Me,$[Me]=Ce)}$.length=Me+1;let ie=n.getParameter(n.UNPACK_ROW_LENGTH),ye=n.getParameter(n.UNPACK_SKIP_PIXELS),Se=n.getParameter(n.UNPACK_SKIP_ROWS);n.pixelStorei(n.UNPACK_ROW_LENGTH,y.width);for(let Q=0,le=$.length;Q<le;Q++){let Ce=$[Q],be=Math.floor(Ce.start/4),oe=Math.ceil(Ce.count/4),ke=be%y.width,C=Math.floor(be/y.width),ee=oe,re=1;n.pixelStorei(n.UNPACK_SKIP_PIXELS,ke),n.pixelStorei(n.UNPACK_SKIP_ROWS,C),t.texSubImage2D(n.TEXTURE_2D,0,ke,C,ee,re,U,H,y.data)}E.clearUpdateRanges(),n.pixelStorei(n.UNPACK_ROW_LENGTH,ie),n.pixelStorei(n.UNPACK_SKIP_PIXELS,ye),n.pixelStorei(n.UNPACK_SKIP_ROWS,Se)}}function q(E,y,U){let H=n.TEXTURE_2D;(y.isDataArrayTexture||y.isCompressedArrayTexture)&&(H=n.TEXTURE_2D_ARRAY),y.isData3DTexture&&(H=n.TEXTURE_3D);let Y=ft(E,y),$=y.source;t.bindTexture(H,E.__webglTexture,n.TEXTURE0+U);let Me=i.get($);if($.version!==Me.__version||Y===!0){t.activeTexture(n.TEXTURE0+U);let ie=tt.getPrimaries(tt.workingColorSpace),ye=y.colorSpace===di?null:tt.getPrimaries(y.colorSpace),Se=y.colorSpace===di||ie===ye?n.NONE:n.BROWSER_DEFAULT_WEBGL;n.pixelStorei(n.UNPACK_FLIP_Y_WEBGL,y.flipY),n.pixelStorei(n.UNPACK_PREMULTIPLY_ALPHA_WEBGL,y.premultiplyAlpha),n.pixelStorei(n.UNPACK_ALIGNMENT,y.unpackAlignment),n.pixelStorei(n.UNPACK_COLORSPACE_CONVERSION_WEBGL,Se);let Q=_(y.image,!1,r.maxTextureSize);Q=Nt(y,Q);let le=s.convert(y.format,y.colorSpace),Ce=s.convert(y.type),be=M(y.internalFormat,le,Ce,y.colorSpace,y.isVideoTexture);je(H,y);let oe,ke=y.mipmaps,C=y.isVideoTexture!==!0,ee=Me.__version===void 0||Y===!0,re=$.dataReady,de=T(y,Q);if(y.isDepthTexture)be=x(y.format===ts,y.type),ee&&(C?t.texStorage2D(n.TEXTURE_2D,1,be,Q.width,Q.height):t.texImage2D(n.TEXTURE_2D,0,be,Q.width,Q.height,0,le,Ce,null));else if(y.isDataTexture)if(ke.length>0){C&&ee&&t.texStorage2D(n.TEXTURE_2D,de,be,ke[0].width,ke[0].height);for(let K=0,X=ke.length;K<X;K++)oe=ke[K],C?re&&t.texSubImage2D(n.TEXTURE_2D,K,0,0,oe.width,oe.height,le,Ce,oe.data):t.texImage2D(n.TEXTURE_2D,K,be,oe.width,oe.height,0,le,Ce,oe.data);y.generateMipmaps=!1}else C?(ee&&t.texStorage2D(n.TEXTURE_2D,de,be,Q.width,Q.height),re&&rt(y,Q,le,Ce)):t.texImage2D(n.TEXTURE_2D,0,be,Q.width,Q.height,0,le,Ce,Q.data);else if(y.isCompressedTexture)if(y.isCompressedArrayTexture){C&&ee&&t.texStorage3D(n.TEXTURE_2D_ARRAY,de,be,ke[0].width,ke[0].height,Q.depth);for(let K=0,X=ke.length;K<X;K++)if(oe=ke[K],y.format!==Tn)if(le!==null)if(C){if(re)if(y.layerUpdates.size>0){let me=Su(oe.width,oe.height,y.format,y.type);for(let Le of y.layerUpdates){let pt=oe.data.subarray(Le*me/oe.data.BYTES_PER_ELEMENT,(Le+1)*me/oe.data.BYTES_PER_ELEMENT);t.compressedTexSubImage3D(n.TEXTURE_2D_ARRAY,K,0,0,Le,oe.width,oe.height,1,le,pt)}y.clearLayerUpdates()}else t.compressedTexSubImage3D(n.TEXTURE_2D_ARRAY,K,0,0,0,oe.width,oe.height,Q.depth,le,oe.data)}else t.compressedTexImage3D(n.TEXTURE_2D_ARRAY,K,be,oe.width,oe.height,Q.depth,0,oe.data,0,0);else console.warn("THREE.WebGLRenderer: Attempt to load unsupported compressed texture format in .uploadTexture()");else C?re&&t.texSubImage3D(n.TEXTURE_2D_ARRAY,K,0,0,0,oe.width,oe.height,Q.depth,le,Ce,oe.data):t.texImage3D(n.TEXTURE_2D_ARRAY,K,be,oe.width,oe.height,Q.depth,0,le,Ce,oe.data)}else{C&&ee&&t.texStorage2D(n.TEXTURE_2D,de,be,ke[0].width,ke[0].height);for(let K=0,X=ke.length;K<X;K++)oe=ke[K],y.format!==Tn?le!==null?C?re&&t.compressedTexSubImage2D(n.TEXTURE_2D,K,0,0,oe.width,oe.height,le,oe.data):t.compressedTexImage2D(n.TEXTURE_2D,K,be,oe.width,oe.height,0,oe.data):console.warn("THREE.WebGLRenderer: Attempt to load unsupported compressed texture format in .uploadTexture()"):C?re&&t.texSubImage2D(n.TEXTURE_2D,K,0,0,oe.width,oe.height,le,Ce,oe.data):t.texImage2D(n.TEXTURE_2D,K,be,oe.width,oe.height,0,le,Ce,oe.data)}else if(y.isDataArrayTexture)if(C){if(ee&&t.texStorage3D(n.TEXTURE_2D_ARRAY,de,be,Q.width,Q.height,Q.depth),re)if(y.layerUpdates.size>0){let K=Su(Q.width,Q.height,y.format,y.type);for(let X of y.layerUpdates){let me=Q.data.subarray(X*K/Q.data.BYTES_PER_ELEMENT,(X+1)*K/Q.data.BYTES_PER_ELEMENT);t.texSubImage3D(n.TEXTURE_2D_ARRAY,0,0,0,X,Q.width,Q.height,1,le,Ce,me)}y.clearLayerUpdates()}else t.texSubImage3D(n.TEXTURE_2D_ARRAY,0,0,0,0,Q.width,Q.height,Q.depth,le,Ce,Q.data)}else t.texImage3D(n.TEXTURE_2D_ARRAY,0,be,Q.width,Q.height,Q.depth,0,le,Ce,Q.data);else if(y.isData3DTexture)C?(ee&&t.texStorage3D(n.TEXTURE_3D,de,be,Q.width,Q.height,Q.depth),re&&t.texSubImage3D(n.TEXTURE_3D,0,0,0,0,Q.width,Q.height,Q.depth,le,Ce,Q.data)):t.texImage3D(n.TEXTURE_3D,0,be,Q.width,Q.height,Q.depth,0,le,Ce,Q.data);else if(y.isFramebufferTexture){if(ee)if(C)t.texStorage2D(n.TEXTURE_2D,de,be,Q.width,Q.height);else{let K=Q.width,X=Q.height;for(let me=0;me<de;me++)t.texImage2D(n.TEXTURE_2D,me,be,K,X,0,le,Ce,null),K>>=1,X>>=1}}else if(ke.length>0){if(C&&ee){let K=At(ke[0]);t.texStorage2D(n.TEXTURE_2D,de,be,K.width,K.height)}for(let K=0,X=ke.length;K<X;K++)oe=ke[K],C?re&&t.texSubImage2D(n.TEXTURE_2D,K,0,0,le,Ce,oe):t.texImage2D(n.TEXTURE_2D,K,be,le,Ce,oe);y.generateMipmaps=!1}else if(C){if(ee){let K=At(Q);t.texStorage2D(n.TEXTURE_2D,de,be,K.width,K.height)}re&&t.texSubImage2D(n.TEXTURE_2D,0,0,0,le,Ce,Q)}else t.texImage2D(n.TEXTURE_2D,0,be,le,Ce,Q);m(y)&&d(H),Me.__version=$.version,y.onUpdate&&y.onUpdate(y)}E.__version=y.version}function Z(E,y,U){if(y.image.length!==6)return;let H=ft(E,y),Y=y.source;t.bindTexture(n.TEXTURE_CUBE_MAP,E.__webglTexture,n.TEXTURE0+U);let $=i.get(Y);if(Y.version!==$.__version||H===!0){t.activeTexture(n.TEXTURE0+U);let Me=tt.getPrimaries(tt.workingColorSpace),ie=y.colorSpace===di?null:tt.getPrimaries(y.colorSpace),ye=y.colorSpace===di||Me===ie?n.NONE:n.BROWSER_DEFAULT_WEBGL;n.pixelStorei(n.UNPACK_FLIP_Y_WEBGL,y.flipY),n.pixelStorei(n.UNPACK_PREMULTIPLY_ALPHA_WEBGL,y.premultiplyAlpha),n.pixelStorei(n.UNPACK_ALIGNMENT,y.unpackAlignment),n.pixelStorei(n.UNPACK_COLORSPACE_CONVERSION_WEBGL,ye);let Se=y.isCompressedTexture||y.image[0].isCompressedTexture,Q=y.image[0]&&y.image[0].isDataTexture,le=[];for(let X=0;X<6;X++)!Se&&!Q?le[X]=_(y.image[X],!0,r.maxCubemapSize):le[X]=Q?y.image[X].image:y.image[X],le[X]=Nt(y,le[X]);let Ce=le[0],be=s.convert(y.format,y.colorSpace),oe=s.convert(y.type),ke=M(y.internalFormat,be,oe,y.colorSpace),C=y.isVideoTexture!==!0,ee=$.__version===void 0||H===!0,re=Y.dataReady,de=T(y,Ce);je(n.TEXTURE_CUBE_MAP,y);let K;if(Se){C&&ee&&t.texStorage2D(n.TEXTURE_CUBE_MAP,de,ke,Ce.width,Ce.height);for(let X=0;X<6;X++){K=le[X].mipmaps;for(let me=0;me<K.length;me++){let Le=K[me];y.format!==Tn?be!==null?C?re&&t.compressedTexSubImage2D(n.TEXTURE_CUBE_MAP_POSITIVE_X+X,me,0,0,Le.width,Le.height,be,Le.data):t.compressedTexImage2D(n.TEXTURE_CUBE_MAP_POSITIVE_X+X,me,ke,Le.width,Le.height,0,Le.data):console.warn("THREE.WebGLRenderer: Attempt to load unsupported compressed texture format in .setTextureCube()"):C?re&&t.texSubImage2D(n.TEXTURE_CUBE_MAP_POSITIVE_X+X,me,0,0,Le.width,Le.height,be,oe,Le.data):t.texImage2D(n.TEXTURE_CUBE_MAP_POSITIVE_X+X,me,ke,Le.width,Le.height,0,be,oe,Le.data)}}}else{if(K=y.mipmaps,C&&ee){K.length>0&&de++;let X=At(le[0]);t.texStorage2D(n.TEXTURE_CUBE_MAP,de,ke,X.width,X.height)}for(let X=0;X<6;X++)if(Q){C?re&&t.texSubImage2D(n.TEXTURE_CUBE_MAP_POSITIVE_X+X,0,0,0,le[X].width,le[X].height,be,oe,le[X].data):t.texImage2D(n.TEXTURE_CUBE_MAP_POSITIVE_X+X,0,ke,le[X].width,le[X].height,0,be,oe,le[X].data);for(let me=0;me<K.length;me++){let pt=K[me].image[X].image;C?re&&t.texSubImage2D(n.TEXTURE_CUBE_MAP_POSITIVE_X+X,me+1,0,0,pt.width,pt.height,be,oe,pt.data):t.texImage2D(n.TEXTURE_CUBE_MAP_POSITIVE_X+X,me+1,ke,pt.width,pt.height,0,be,oe,pt.data)}}else{C?re&&t.texSubImage2D(n.TEXTURE_CUBE_MAP_POSITIVE_X+X,0,0,0,be,oe,le[X]):t.texImage2D(n.TEXTURE_CUBE_MAP_POSITIVE_X+X,0,ke,be,oe,le[X]);for(let me=0;me<K.length;me++){let Le=K[me];C?re&&t.texSubImage2D(n.TEXTURE_CUBE_MAP_POSITIVE_X+X,me+1,0,0,be,oe,Le.image[X]):t.texImage2D(n.TEXTURE_CUBE_MAP_POSITIVE_X+X,me+1,ke,be,oe,Le.image[X])}}}m(y)&&d(n.TEXTURE_CUBE_MAP),$.__version=Y.version,y.onUpdate&&y.onUpdate(y)}E.__version=y.version}function fe(E,y,U,H,Y,$){let Me=s.convert(U.format,U.colorSpace),ie=s.convert(U.type),ye=M(U.internalFormat,Me,ie,U.colorSpace),Se=i.get(y),Q=i.get(U);if(Q.__renderTarget=y,!Se.__hasExternalTextures){let le=Math.max(1,y.width>>$),Ce=Math.max(1,y.height>>$);Y===n.TEXTURE_3D||Y===n.TEXTURE_2D_ARRAY?t.texImage3D(Y,$,ye,le,Ce,y.depth,0,Me,ie,null):t.texImage2D(Y,$,ye,le,Ce,0,Me,ie,null)}t.bindFramebuffer(n.FRAMEBUFFER,E),_e(y)?o.framebufferTexture2DMultisampleEXT(n.FRAMEBUFFER,H,Y,Q.__webglTexture,0,bt(y)):(Y===n.TEXTURE_2D||Y>=n.TEXTURE_CUBE_MAP_POSITIVE_X&&Y<=n.TEXTURE_CUBE_MAP_NEGATIVE_Z)&&n.framebufferTexture2D(n.FRAMEBUFFER,H,Y,Q.__webglTexture,$),t.bindFramebuffer(n.FRAMEBUFFER,null)}function Ne(E,y,U){if(n.bindRenderbuffer(n.RENDERBUFFER,E),y.depthBuffer){let H=y.depthTexture,Y=H&&H.isDepthTexture?H.type:null,$=x(y.stencilBuffer,Y),Me=y.stencilBuffer?n.DEPTH_STENCIL_ATTACHMENT:n.DEPTH_ATTACHMENT,ie=bt(y);_e(y)?o.renderbufferStorageMultisampleEXT(n.RENDERBUFFER,ie,$,y.width,y.height):U?n.renderbufferStorageMultisample(n.RENDERBUFFER,ie,$,y.width,y.height):n.renderbufferStorage(n.RENDERBUFFER,$,y.width,y.height),n.framebufferRenderbuffer(n.FRAMEBUFFER,Me,n.RENDERBUFFER,E)}else{let H=y.textures;for(let Y=0;Y<H.length;Y++){let $=H[Y],Me=s.convert($.format,$.colorSpace),ie=s.convert($.type),ye=M($.internalFormat,Me,ie,$.colorSpace),Se=bt(y);U&&_e(y)===!1?n.renderbufferStorageMultisample(n.RENDERBUFFER,Se,ye,y.width,y.height):_e(y)?o.renderbufferStorageMultisampleEXT(n.RENDERBUFFER,Se,ye,y.width,y.height):n.renderbufferStorage(n.RENDERBUFFER,ye,y.width,y.height)}}n.bindRenderbuffer(n.RENDERBUFFER,null)}function Ee(E,y){if(y&&y.isWebGLCubeRenderTarget)throw new Error("Depth Texture with cube render targets is not supported");if(t.bindFramebuffer(n.FRAMEBUFFER,E),!(y.depthTexture&&y.depthTexture.isDepthTexture))throw new Error("renderTarget.depthTexture must be an instance of THREE.DepthTexture");let H=i.get(y.depthTexture);H.__renderTarget=y,(!H.__webglTexture||y.depthTexture.image.width!==y.width||y.depthTexture.image.height!==y.height)&&(y.depthTexture.image.width=y.width,y.depthTexture.image.height=y.height,y.depthTexture.needsUpdate=!0),j(y.depthTexture,0);let Y=H.__webglTexture,$=bt(y);if(y.depthTexture.format===Gr)_e(y)?o.framebufferTexture2DMultisampleEXT(n.FRAMEBUFFER,n.DEPTH_ATTACHMENT,n.TEXTURE_2D,Y,0,$):n.framebufferTexture2D(n.FRAMEBUFFER,n.DEPTH_ATTACHMENT,n.TEXTURE_2D,Y,0);else if(y.depthTexture.format===ts)_e(y)?o.framebufferTexture2DMultisampleEXT(n.FRAMEBUFFER,n.DEPTH_STENCIL_ATTACHMENT,n.TEXTURE_2D,Y,0,$):n.framebufferTexture2D(n.FRAMEBUFFER,n.DEPTH_STENCIL_ATTACHMENT,n.TEXTURE_2D,Y,0);else throw new Error("Unknown depthTexture format")}function Je(E){let y=i.get(E),U=E.isWebGLCubeRenderTarget===!0;if(y.__boundDepthTexture!==E.depthTexture){let H=E.depthTexture;if(y.__depthDisposeCallback&&y.__depthDisposeCallback(),H){let Y=()=>{delete y.__boundDepthTexture,delete y.__depthDisposeCallback,H.removeEventListener("dispose",Y)};H.addEventListener("dispose",Y),y.__depthDisposeCallback=Y}y.__boundDepthTexture=H}if(E.depthTexture&&!y.__autoAllocateDepthBuffer){if(U)throw new Error("target.depthTexture not supported in Cube render targets");let H=E.texture.mipmaps;H&&H.length>0?Ee(y.__webglFramebuffer[0],E):Ee(y.__webglFramebuffer,E)}else if(U){y.__webglDepthbuffer=[];for(let H=0;H<6;H++)if(t.bindFramebuffer(n.FRAMEBUFFER,y.__webglFramebuffer[H]),y.__webglDepthbuffer[H]===void 0)y.__webglDepthbuffer[H]=n.createRenderbuffer(),Ne(y.__webglDepthbuffer[H],E,!1);else{let Y=E.stencilBuffer?n.DEPTH_STENCIL_ATTACHMENT:n.DEPTH_ATTACHMENT,$=y.__webglDepthbuffer[H];n.bindRenderbuffer(n.RENDERBUFFER,$),n.framebufferRenderbuffer(n.FRAMEBUFFER,Y,n.RENDERBUFFER,$)}}else{let H=E.texture.mipmaps;if(H&&H.length>0?t.bindFramebuffer(n.FRAMEBUFFER,y.__webglFramebuffer[0]):t.bindFramebuffer(n.FRAMEBUFFER,y.__webglFramebuffer),y.__webglDepthbuffer===void 0)y.__webglDepthbuffer=n.createRenderbuffer(),Ne(y.__webglDepthbuffer,E,!1);else{let Y=E.stencilBuffer?n.DEPTH_STENCIL_ATTACHMENT:n.DEPTH_ATTACHMENT,$=y.__webglDepthbuffer;n.bindRenderbuffer(n.RENDERBUFFER,$),n.framebufferRenderbuffer(n.FRAMEBUFFER,Y,n.RENDERBUFFER,$)}}t.bindFramebuffer(n.FRAMEBUFFER,null)}function Bt(E,y,U){let H=i.get(E);y!==void 0&&fe(H.__webglFramebuffer,E,E.texture,n.COLOR_ATTACHMENT0,n.TEXTURE_2D,0),U!==void 0&&Je(E)}function A(E){let y=E.texture,U=i.get(E),H=i.get(y);E.addEventListener("dispose",P);let Y=E.textures,$=E.isWebGLCubeRenderTarget===!0,Me=Y.length>1;if(Me||(H.__webglTexture===void 0&&(H.__webglTexture=n.createTexture()),H.__version=y.version,a.memory.textures++),$){U.__webglFramebuffer=[];for(let ie=0;ie<6;ie++)if(y.mipmaps&&y.mipmaps.length>0){U.__webglFramebuffer[ie]=[];for(let ye=0;ye<y.mipmaps.length;ye++)U.__webglFramebuffer[ie][ye]=n.createFramebuffer()}else U.__webglFramebuffer[ie]=n.createFramebuffer()}else{if(y.mipmaps&&y.mipmaps.length>0){U.__webglFramebuffer=[];for(let ie=0;ie<y.mipmaps.length;ie++)U.__webglFramebuffer[ie]=n.createFramebuffer()}else U.__webglFramebuffer=n.createFramebuffer();if(Me)for(let ie=0,ye=Y.length;ie<ye;ie++){let Se=i.get(Y[ie]);Se.__webglTexture===void 0&&(Se.__webglTexture=n.createTexture(),a.memory.textures++)}if(E.samples>0&&_e(E)===!1){U.__webglMultisampledFramebuffer=n.createFramebuffer(),U.__webglColorRenderbuffer=[],t.bindFramebuffer(n.FRAMEBUFFER,U.__webglMultisampledFramebuffer);for(let ie=0;ie<Y.length;ie++){let ye=Y[ie];U.__webglColorRenderbuffer[ie]=n.createRenderbuffer(),n.bindRenderbuffer(n.RENDERBUFFER,U.__webglColorRenderbuffer[ie]);let Se=s.convert(ye.format,ye.colorSpace),Q=s.convert(ye.type),le=M(ye.internalFormat,Se,Q,ye.colorSpace,E.isXRRenderTarget===!0),Ce=bt(E);n.renderbufferStorageMultisample(n.RENDERBUFFER,Ce,le,E.width,E.height),n.framebufferRenderbuffer(n.FRAMEBUFFER,n.COLOR_ATTACHMENT0+ie,n.RENDERBUFFER,U.__webglColorRenderbuffer[ie])}n.bindRenderbuffer(n.RENDERBUFFER,null),E.depthBuffer&&(U.__webglDepthRenderbuffer=n.createRenderbuffer(),Ne(U.__webglDepthRenderbuffer,E,!0)),t.bindFramebuffer(n.FRAMEBUFFER,null)}}if($){t.bindTexture(n.TEXTURE_CUBE_MAP,H.__webglTexture),je(n.TEXTURE_CUBE_MAP,y);for(let ie=0;ie<6;ie++)if(y.mipmaps&&y.mipmaps.length>0)for(let ye=0;ye<y.mipmaps.length;ye++)fe(U.__webglFramebuffer[ie][ye],E,y,n.COLOR_ATTACHMENT0,n.TEXTURE_CUBE_MAP_POSITIVE_X+ie,ye);else fe(U.__webglFramebuffer[ie],E,y,n.COLOR_ATTACHMENT0,n.TEXTURE_CUBE_MAP_POSITIVE_X+ie,0);m(y)&&d(n.TEXTURE_CUBE_MAP),t.unbindTexture()}else if(Me){for(let ie=0,ye=Y.length;ie<ye;ie++){let Se=Y[ie],Q=i.get(Se),le=n.TEXTURE_2D;(E.isWebGL3DRenderTarget||E.isWebGLArrayRenderTarget)&&(le=E.isWebGL3DRenderTarget?n.TEXTURE_3D:n.TEXTURE_2D_ARRAY),t.bindTexture(le,Q.__webglTexture),je(le,Se),fe(U.__webglFramebuffer,E,Se,n.COLOR_ATTACHMENT0+ie,le,0),m(Se)&&d(le)}t.unbindTexture()}else{let ie=n.TEXTURE_2D;if((E.isWebGL3DRenderTarget||E.isWebGLArrayRenderTarget)&&(ie=E.isWebGL3DRenderTarget?n.TEXTURE_3D:n.TEXTURE_2D_ARRAY),t.bindTexture(ie,H.__webglTexture),je(ie,y),y.mipmaps&&y.mipmaps.length>0)for(let ye=0;ye<y.mipmaps.length;ye++)fe(U.__webglFramebuffer[ye],E,y,n.COLOR_ATTACHMENT0,ie,ye);else fe(U.__webglFramebuffer,E,y,n.COLOR_ATTACHMENT0,ie,0);m(y)&&d(ie),t.unbindTexture()}E.depthBuffer&&Je(E)}function St(E){let y=E.textures;for(let U=0,H=y.length;U<H;U++){let Y=y[U];if(m(Y)){let $=w(E),Me=i.get(Y).__webglTexture;t.bindTexture($,Me),d($),t.unbindTexture()}}}let Oe=[],Pe=[];function ge(E){if(E.samples>0){if(_e(E)===!1){let y=E.textures,U=E.width,H=E.height,Y=n.COLOR_BUFFER_BIT,$=E.stencilBuffer?n.DEPTH_STENCIL_ATTACHMENT:n.DEPTH_ATTACHMENT,Me=i.get(E),ie=y.length>1;if(ie)for(let Se=0;Se<y.length;Se++)t.bindFramebuffer(n.FRAMEBUFFER,Me.__webglMultisampledFramebuffer),n.framebufferRenderbuffer(n.FRAMEBUFFER,n.COLOR_ATTACHMENT0+Se,n.RENDERBUFFER,null),t.bindFramebuffer(n.FRAMEBUFFER,Me.__webglFramebuffer),n.framebufferTexture2D(n.DRAW_FRAMEBUFFER,n.COLOR_ATTACHMENT0+Se,n.TEXTURE_2D,null,0);t.bindFramebuffer(n.READ_FRAMEBUFFER,Me.__webglMultisampledFramebuffer);let ye=E.texture.mipmaps;ye&&ye.length>0?t.bindFramebuffer(n.DRAW_FRAMEBUFFER,Me.__webglFramebuffer[0]):t.bindFramebuffer(n.DRAW_FRAMEBUFFER,Me.__webglFramebuffer);for(let Se=0;Se<y.length;Se++){if(E.resolveDepthBuffer&&(E.depthBuffer&&(Y|=n.DEPTH_BUFFER_BIT),E.stencilBuffer&&E.resolveStencilBuffer&&(Y|=n.STENCIL_BUFFER_BIT)),ie){n.framebufferRenderbuffer(n.READ_FRAMEBUFFER,n.COLOR_ATTACHMENT0,n.RENDERBUFFER,Me.__webglColorRenderbuffer[Se]);let Q=i.get(y[Se]).__webglTexture;n.framebufferTexture2D(n.DRAW_FRAMEBUFFER,n.COLOR_ATTACHMENT0,n.TEXTURE_2D,Q,0)}n.blitFramebuffer(0,0,U,H,0,0,U,H,Y,n.NEAREST),c===!0&&(Oe.length=0,Pe.length=0,Oe.push(n.COLOR_ATTACHMENT0+Se),E.depthBuffer&&E.resolveDepthBuffer===!1&&(Oe.push($),Pe.push($),n.invalidateFramebuffer(n.DRAW_FRAMEBUFFER,Pe)),n.invalidateFramebuffer(n.READ_FRAMEBUFFER,Oe))}if(t.bindFramebuffer(n.READ_FRAMEBUFFER,null),t.bindFramebuffer(n.DRAW_FRAMEBUFFER,null),ie)for(let Se=0;Se<y.length;Se++){t.bindFramebuffer(n.FRAMEBUFFER,Me.__webglMultisampledFramebuffer),n.framebufferRenderbuffer(n.FRAMEBUFFER,n.COLOR_ATTACHMENT0+Se,n.RENDERBUFFER,Me.__webglColorRenderbuffer[Se]);let Q=i.get(y[Se]).__webglTexture;t.bindFramebuffer(n.FRAMEBUFFER,Me.__webglFramebuffer),n.framebufferTexture2D(n.DRAW_FRAMEBUFFER,n.COLOR_ATTACHMENT0+Se,n.TEXTURE_2D,Q,0)}t.bindFramebuffer(n.DRAW_FRAMEBUFFER,Me.__webglMultisampledFramebuffer)}else if(E.depthBuffer&&E.resolveDepthBuffer===!1&&c){let y=E.stencilBuffer?n.DEPTH_STENCIL_ATTACHMENT:n.DEPTH_ATTACHMENT;n.invalidateFramebuffer(n.DRAW_FRAMEBUFFER,[y])}}}function bt(E){return Math.min(r.maxSamples,E.samples)}function _e(E){let y=i.get(E);return E.samples>0&&e.has("WEBGL_multisampled_render_to_texture")===!0&&y.__useRenderToTexture!==!1}function $e(E){let y=a.render.frame;l.get(E)!==y&&(l.set(E,y),E.update())}function Nt(E,y){let U=E.colorSpace,H=E.format,Y=E.type;return E.isCompressedTexture===!0||E.isVideoTexture===!0||U!==Qi&&U!==di&&(tt.getTransfer(U)===ot?(H!==Tn||Y!==Zn)&&console.warn("THREE.WebGLTextures: sRGB encoded textures have to use RGBAFormat and UnsignedByteType."):console.error("THREE.WebGLTextures: Unsupported texture color space:",U)),y}function At(E){return typeof HTMLImageElement<"u"&&E instanceof HTMLImageElement?(u.width=E.naturalWidth||E.width,u.height=E.naturalHeight||E.height):typeof VideoFrame<"u"&&E instanceof VideoFrame?(u.width=E.displayWidth,u.height=E.displayHeight):(u.width=E.width,u.height=E.height),u}this.allocateTextureUnit=V,this.resetTextureUnits=k,this.setTexture2D=j,this.setTexture2DArray=G,this.setTexture3D=ne,this.setTextureCube=z,this.rebindTextures=Bt,this.setupRenderTarget=A,this.updateRenderTargetMipmap=St,this.updateMultisampleRenderTarget=ge,this.setupDepthRenderbuffer=Je,this.setupFrameBufferTexture=fe,this.useMultisampledRTT=_e}function dM(n,e){function t(i,r=di){let s,a=tt.getTransfer(r);if(i===Zn)return n.UNSIGNED_BYTE;if(i===Wo)return n.UNSIGNED_SHORT_4_4_4_4;if(i===qo)return n.UNSIGNED_SHORT_5_5_5_1;if(i===lu)return n.UNSIGNED_INT_5_9_9_9_REV;if(i===uu)return n.UNSIGNED_INT_10F_11F_11F_REV;if(i===ou)return n.BYTE;if(i===cu)return n.SHORT;if(i===Jr)return n.UNSIGNED_SHORT;if(i===Go)return n.INT;if(i===Ni)return n.UNSIGNED_INT;if(i===Bn)return n.FLOAT;if(i===Qr)return n.HALF_FLOAT;if(i===hu)return n.ALPHA;if(i===du)return n.RGB;if(i===Tn)return n.RGBA;if(i===Gr)return n.DEPTH_COMPONENT;if(i===ts)return n.DEPTH_STENCIL;if(i===Xo)return n.RED;if(i===jo)return n.RED_INTEGER;if(i===fu)return n.RG;if(i===Yo)return n.RG_INTEGER;if(i===Zo)return n.RGBA_INTEGER;if(i===ta||i===na||i===ia||i===ra)if(a===ot)if(s=e.get("WEBGL_compressed_texture_s3tc_srgb"),s!==null){if(i===ta)return s.COMPRESSED_SRGB_S3TC_DXT1_EXT;if(i===na)return s.COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;if(i===ia)return s.COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;if(i===ra)return s.COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT}else return null;else if(s=e.get("WEBGL_compressed_texture_s3tc"),s!==null){if(i===ta)return s.COMPRESSED_RGB_S3TC_DXT1_EXT;if(i===na)return s.COMPRESSED_RGBA_S3TC_DXT1_EXT;if(i===ia)return s.COMPRESSED_RGBA_S3TC_DXT3_EXT;if(i===ra)return s.COMPRESSED_RGBA_S3TC_DXT5_EXT}else return null;if(i===Ko||i===Jo||i===Qo||i===ec)if(s=e.get("WEBGL_compressed_texture_pvrtc"),s!==null){if(i===Ko)return s.COMPRESSED_RGB_PVRTC_4BPPV1_IMG;if(i===Jo)return s.COMPRESSED_RGB_PVRTC_2BPPV1_IMG;if(i===Qo)return s.COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;if(i===ec)return s.COMPRESSED_RGBA_PVRTC_2BPPV1_IMG}else return null;if(i===tc||i===nc||i===ic)if(s=e.get("WEBGL_compressed_texture_etc"),s!==null){if(i===tc||i===nc)return a===ot?s.COMPRESSED_SRGB8_ETC2:s.COMPRESSED_RGB8_ETC2;if(i===ic)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:s.COMPRESSED_RGBA8_ETC2_EAC}else return null;if(i===rc||i===sc||i===ac||i===oc||i===cc||i===lc||i===uc||i===hc||i===dc||i===fc||i===pc||i===mc||i===gc||i===_c)if(s=e.get("WEBGL_compressed_texture_astc"),s!==null){if(i===rc)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:s.COMPRESSED_RGBA_ASTC_4x4_KHR;if(i===sc)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:s.COMPRESSED_RGBA_ASTC_5x4_KHR;if(i===ac)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:s.COMPRESSED_RGBA_ASTC_5x5_KHR;if(i===oc)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:s.COMPRESSED_RGBA_ASTC_6x5_KHR;if(i===cc)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:s.COMPRESSED_RGBA_ASTC_6x6_KHR;if(i===lc)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:s.COMPRESSED_RGBA_ASTC_8x5_KHR;if(i===uc)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:s.COMPRESSED_RGBA_ASTC_8x6_KHR;if(i===hc)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:s.COMPRESSED_RGBA_ASTC_8x8_KHR;if(i===dc)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:s.COMPRESSED_RGBA_ASTC_10x5_KHR;if(i===fc)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:s.COMPRESSED_RGBA_ASTC_10x6_KHR;if(i===pc)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:s.COMPRESSED_RGBA_ASTC_10x8_KHR;if(i===mc)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:s.COMPRESSED_RGBA_ASTC_10x10_KHR;if(i===gc)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:s.COMPRESSED_RGBA_ASTC_12x10_KHR;if(i===_c)return a===ot?s.COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:s.COMPRESSED_RGBA_ASTC_12x12_KHR}else return null;if(i===yc||i===vc||i===xc)if(s=e.get("EXT_texture_compression_bptc"),s!==null){if(i===yc)return a===ot?s.COMPRESSED_SRGB_ALPHA_BPTC_UNORM_EXT:s.COMPRESSED_RGBA_BPTC_UNORM_EXT;if(i===vc)return s.COMPRESSED_RGB_BPTC_SIGNED_FLOAT_EXT;if(i===xc)return s.COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_EXT}else return null;if(i===Sc||i===bc||i===Mc||i===Ec)if(s=e.get("EXT_texture_compression_rgtc"),s!==null){if(i===Sc)return s.COMPRESSED_RED_RGTC1_EXT;if(i===bc)return s.COMPRESSED_SIGNED_RED_RGTC1_EXT;if(i===Mc)return s.COMPRESSED_RED_GREEN_RGTC2_EXT;if(i===Ec)return s.COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT}else return null;return i===es?n.UNSIGNED_INT_24_8:n[i]!==void 0?n[i]:null}return{convert:t}}var fM=`
void main() {

	gl_Position = vec4( position, 1.0 );

}`,pM=`
uniform sampler2DArray depthColor;
uniform float depthWidth;
uniform float depthHeight;

void main() {

	vec2 coord = vec2( gl_FragCoord.x / depthWidth, gl_FragCoord.y / depthHeight );

	if ( coord.x >= 1.0 ) {

		gl_FragDepth = texture( depthColor, vec3( coord.x - 1.0, coord.y, 1 ) ).r;

	} else {

		gl_FragDepth = texture( depthColor, vec3( coord.x, coord.y, 0 ) ).r;

	}

}`,Lu=class{constructor(){this.texture=null,this.mesh=null,this.depthNear=0,this.depthFar=0}init(e,t){if(this.texture===null){let i=new Xs(e.texture);(e.depthNear!==t.depthNear||e.depthFar!==t.depthFar)&&(this.depthNear=e.depthNear,this.depthFar=e.depthFar),this.texture=i}}getMesh(e){if(this.texture!==null&&this.mesh===null){let t=e.cameras[0].viewport,i=new Fn({vertexShader:fM,fragmentShader:pM,uniforms:{depthColor:{value:this.texture},depthWidth:{value:t.z},depthHeight:{value:t.w}}});this.mesh=new Ht(new js(20,20),i)}return this.mesh}reset(){this.texture=null,this.mesh=null}getDepthTexture(){return this.texture}},Uu=class extends Gn{constructor(e,t){super();let i=this,r=null,s=1,a=null,o="local-floor",c=1,u=null,l=null,h=null,f=null,p=null,g=null,_=typeof XRWebGLBinding<"u",m=new Lu,d={},w=t.getContextAttributes(),M=null,x=null,T=[],R=[],P=new Ue,O=null,b=new $t;b.viewport=new Tt;let S=new $t;S.viewport=new Tt;let I=[b,S],k=new No,V=null,W=null;this.cameraAutoUpdate=!0,this.enabled=!1,this.isPresenting=!1,this.getController=function(q){let Z=T[q];return Z===void 0&&(Z=new Zr,T[q]=Z),Z.getTargetRaySpace()},this.getControllerGrip=function(q){let Z=T[q];return Z===void 0&&(Z=new Zr,T[q]=Z),Z.getGripSpace()},this.getHand=function(q){let Z=T[q];return Z===void 0&&(Z=new Zr,T[q]=Z),Z.getHandSpace()};function j(q){let Z=R.indexOf(q.inputSource);if(Z===-1)return;let fe=T[Z];fe!==void 0&&(fe.update(q.inputSource,q.frame,u||a),fe.dispatchEvent({type:q.type,data:q.inputSource}))}function G(){r.removeEventListener("select",j),r.removeEventListener("selectstart",j),r.removeEventListener("selectend",j),r.removeEventListener("squeeze",j),r.removeEventListener("squeezestart",j),r.removeEventListener("squeezeend",j),r.removeEventListener("end",G),r.removeEventListener("inputsourceschange",ne);for(let q=0;q<T.length;q++){let Z=R[q];Z!==null&&(R[q]=null,T[q].disconnect(Z))}V=null,W=null,m.reset();for(let q in d)delete d[q];e.setRenderTarget(M),p=null,f=null,h=null,r=null,x=null,rt.stop(),i.isPresenting=!1,e.setPixelRatio(O),e.setSize(P.width,P.height,!1),i.dispatchEvent({type:"sessionend"})}this.setFramebufferScaleFactor=function(q){s=q,i.isPresenting===!0&&console.warn("THREE.WebXRManager: Cannot change framebuffer scale while presenting.")},this.setReferenceSpaceType=function(q){o=q,i.isPresenting===!0&&console.warn("THREE.WebXRManager: Cannot change reference space type while presenting.")},this.getReferenceSpace=function(){return u||a},this.setReferenceSpace=function(q){u=q},this.getBaseLayer=function(){return f!==null?f:p},this.getBinding=function(){return h===null&&_&&(h=new XRWebGLBinding(r,t)),h},this.getFrame=function(){return g},this.getSession=function(){return r},this.setSession=async function(q){if(r=q,r!==null){if(M=e.getRenderTarget(),r.addEventListener("select",j),r.addEventListener("selectstart",j),r.addEventListener("selectend",j),r.addEventListener("squeeze",j),r.addEventListener("squeezestart",j),r.addEventListener("squeezeend",j),r.addEventListener("end",G),r.addEventListener("inputsourceschange",ne),w.xrCompatible!==!0&&await t.makeXRCompatible(),O=e.getPixelRatio(),e.getSize(P),_&&"createProjectionLayer"in XRWebGLBinding.prototype){let fe=null,Ne=null,Ee=null;w.depth&&(Ee=w.stencil?t.DEPTH24_STENCIL8:t.DEPTH_COMPONENT24,fe=w.stencil?ts:Gr,Ne=w.stencil?es:Ni);let Je={colorFormat:t.RGBA8,depthFormat:Ee,scaleFactor:s};h=this.getBinding(),f=h.createProjectionLayer(Je),r.updateRenderState({layers:[f]}),e.setPixelRatio(1),e.setSize(f.textureWidth,f.textureHeight,!1),x=new Wn(f.textureWidth,f.textureHeight,{format:Tn,type:Zn,depthTexture:new qs(f.textureWidth,f.textureHeight,Ne,void 0,void 0,void 0,void 0,void 0,void 0,fe),stencilBuffer:w.stencil,colorSpace:e.outputColorSpace,samples:w.antialias?4:0,resolveDepthBuffer:f.ignoreDepthValues===!1,resolveStencilBuffer:f.ignoreDepthValues===!1})}else{let fe={antialias:w.antialias,alpha:!0,depth:w.depth,stencil:w.stencil,framebufferScaleFactor:s};p=new XRWebGLLayer(r,t,fe),r.updateRenderState({baseLayer:p}),e.setPixelRatio(1),e.setSize(p.framebufferWidth,p.framebufferHeight,!1),x=new Wn(p.framebufferWidth,p.framebufferHeight,{format:Tn,type:Zn,colorSpace:e.outputColorSpace,stencilBuffer:w.stencil,resolveDepthBuffer:p.ignoreDepthValues===!1,resolveStencilBuffer:p.ignoreDepthValues===!1})}x.isXRRenderTarget=!0,this.setFoveation(c),u=null,a=await r.requestReferenceSpace(o),rt.setContext(r),rt.start(),i.isPresenting=!0,i.dispatchEvent({type:"sessionstart"})}},this.getEnvironmentBlendMode=function(){if(r!==null)return r.environmentBlendMode},this.getDepthTexture=function(){return m.getDepthTexture()};function ne(q){for(let Z=0;Z<q.removed.length;Z++){let fe=q.removed[Z],Ne=R.indexOf(fe);Ne>=0&&(R[Ne]=null,T[Ne].disconnect(fe))}for(let Z=0;Z<q.added.length;Z++){let fe=q.added[Z],Ne=R.indexOf(fe);if(Ne===-1){for(let Je=0;Je<T.length;Je++)if(Je>=R.length){R.push(fe),Ne=Je;break}else if(R[Je]===null){R[Je]=fe,Ne=Je;break}if(Ne===-1)break}let Ee=T[Ne];Ee&&Ee.connect(fe)}}let z=new L,ae=new L;function ue(q,Z,fe){z.setFromMatrixPosition(Z.matrixWorld),ae.setFromMatrixPosition(fe.matrixWorld);let Ne=z.distanceTo(ae),Ee=Z.projectionMatrix.elements,Je=fe.projectionMatrix.elements,Bt=Ee[14]/(Ee[10]-1),A=Ee[14]/(Ee[10]+1),St=(Ee[9]+1)/Ee[5],Oe=(Ee[9]-1)/Ee[5],Pe=(Ee[8]-1)/Ee[0],ge=(Je[8]+1)/Je[0],bt=Bt*Pe,_e=Bt*ge,$e=Ne/(-Pe+ge),Nt=$e*-Pe;if(Z.matrixWorld.decompose(q.position,q.quaternion,q.scale),q.translateX(Nt),q.translateZ($e),q.matrixWorld.compose(q.position,q.quaternion,q.scale),q.matrixWorldInverse.copy(q.matrixWorld).invert(),Ee[10]===-1)q.projectionMatrix.copy(Z.projectionMatrix),q.projectionMatrixInverse.copy(Z.projectionMatrixInverse);else{let At=Bt+$e,E=A+$e,y=bt-Nt,U=_e+(Ne-Nt),H=St*A/E*At,Y=Oe*A/E*At;q.projectionMatrix.makePerspective(y,U,H,Y,At,E),q.projectionMatrixInverse.copy(q.projectionMatrix).invert()}}function we(q,Z){Z===null?q.matrixWorld.copy(q.matrix):q.matrixWorld.multiplyMatrices(Z.matrixWorld,q.matrix),q.matrixWorldInverse.copy(q.matrixWorld).invert()}this.updateCamera=function(q){if(r===null)return;let Z=q.near,fe=q.far;m.texture!==null&&(m.depthNear>0&&(Z=m.depthNear),m.depthFar>0&&(fe=m.depthFar)),k.near=S.near=b.near=Z,k.far=S.far=b.far=fe,(V!==k.near||W!==k.far)&&(r.updateRenderState({depthNear:k.near,depthFar:k.far}),V=k.near,W=k.far),k.layers.mask=q.layers.mask|6,b.layers.mask=k.layers.mask&3,S.layers.mask=k.layers.mask&5;let Ne=q.parent,Ee=k.cameras;we(k,Ne);for(let Je=0;Je<Ee.length;Je++)we(Ee[Je],Ne);Ee.length===2?ue(k,b,S):k.projectionMatrix.copy(b.projectionMatrix),je(q,k,Ne)};function je(q,Z,fe){fe===null?q.matrix.copy(Z.matrixWorld):(q.matrix.copy(fe.matrixWorld),q.matrix.invert(),q.matrix.multiply(Z.matrixWorld)),q.matrix.decompose(q.position,q.quaternion,q.scale),q.updateMatrixWorld(!0),q.projectionMatrix.copy(Z.projectionMatrix),q.projectionMatrixInverse.copy(Z.projectionMatrixInverse),q.isPerspectiveCamera&&(q.fov=Wr*2*Math.atan(1/q.projectionMatrix.elements[5]),q.zoom=1)}this.getCamera=function(){return k},this.getFoveation=function(){if(!(f===null&&p===null))return c},this.setFoveation=function(q){c=q,f!==null&&(f.fixedFoveation=q),p!==null&&p.fixedFoveation!==void 0&&(p.fixedFoveation=q)},this.hasDepthSensing=function(){return m.texture!==null},this.getDepthSensingMesh=function(){return m.getMesh(k)},this.getCameraTexture=function(q){return d[q]};let ft=null;function xt(q,Z){if(l=Z.getViewerPose(u||a),g=Z,l!==null){let fe=l.views;p!==null&&(e.setRenderTargetFramebuffer(x,p.framebuffer),e.setRenderTarget(x));let Ne=!1;fe.length!==k.cameras.length&&(k.cameras.length=0,Ne=!0);for(let A=0;A<fe.length;A++){let St=fe[A],Oe=null;if(p!==null)Oe=p.getViewport(St);else{let ge=h.getViewSubImage(f,St);Oe=ge.viewport,A===0&&(e.setRenderTargetTextures(x,ge.colorTexture,ge.depthStencilTexture),e.setRenderTarget(x))}let Pe=I[A];Pe===void 0&&(Pe=new $t,Pe.layers.enable(A),Pe.viewport=new Tt,I[A]=Pe),Pe.matrix.fromArray(St.transform.matrix),Pe.matrix.decompose(Pe.position,Pe.quaternion,Pe.scale),Pe.projectionMatrix.fromArray(St.projectionMatrix),Pe.projectionMatrixInverse.copy(Pe.projectionMatrix).invert(),Pe.viewport.set(Oe.x,Oe.y,Oe.width,Oe.height),A===0&&(k.matrix.copy(Pe.matrix),k.matrix.decompose(k.position,k.quaternion,k.scale)),Ne===!0&&k.cameras.push(Pe)}let Ee=r.enabledFeatures;if(Ee&&Ee.includes("depth-sensing")&&r.depthUsage=="gpu-optimized"&&_){h=i.getBinding();let A=h.getDepthInformation(fe[0]);A&&A.isValid&&A.texture&&m.init(A,r.renderState)}if(Ee&&Ee.includes("camera-access")&&_){e.state.unbindTexture(),h=i.getBinding();for(let A=0;A<fe.length;A++){let St=fe[A].camera;if(St){let Oe=d[St];Oe||(Oe=new Xs,d[St]=Oe);let Pe=h.getCameraImage(St);Oe.sourceTexture=Pe}}}}for(let fe=0;fe<T.length;fe++){let Ne=R[fe],Ee=T[fe];Ne!==null&&Ee!==void 0&&Ee.update(Ne,Z,u||a)}ft&&ft(q,Z),Z.detectedPlanes&&i.dispatchEvent({type:"planesdetected",data:Z}),g=null}let rt=new Xp;rt.setAnimationLoop(xt),this.setAnimationLoop=function(q){ft=q},this.dispose=function(){}}},or=new Xn,mM=new _t;function gM(n,e){function t(m,d){m.matrixAutoUpdate===!0&&m.updateMatrix(),d.value.copy(m.matrix)}function i(m,d){d.color.getRGB(m.fogColor.value,yu(n)),d.isFog?(m.fogNear.value=d.near,m.fogFar.value=d.far):d.isFogExp2&&(m.fogDensity.value=d.density)}function r(m,d,w,M,x){d.isMeshBasicMaterial||d.isMeshLambertMaterial?s(m,d):d.isMeshToonMaterial?(s(m,d),h(m,d)):d.isMeshPhongMaterial?(s(m,d),l(m,d)):d.isMeshStandardMaterial?(s(m,d),f(m,d),d.isMeshPhysicalMaterial&&p(m,d,x)):d.isMeshMatcapMaterial?(s(m,d),g(m,d)):d.isMeshDepthMaterial?s(m,d):d.isMeshDistanceMaterial?(s(m,d),_(m,d)):d.isMeshNormalMaterial?s(m,d):d.isLineBasicMaterial?(a(m,d),d.isLineDashedMaterial&&o(m,d)):d.isPointsMaterial?c(m,d,w,M):d.isSpriteMaterial?u(m,d):d.isShadowMaterial?(m.color.value.copy(d.color),m.opacity.value=d.opacity):d.isShaderMaterial&&(d.uniformsNeedUpdate=!1)}function s(m,d){m.opacity.value=d.opacity,d.color&&m.diffuse.value.copy(d.color),d.emissive&&m.emissive.value.copy(d.emissive).multiplyScalar(d.emissiveIntensity),d.map&&(m.map.value=d.map,t(d.map,m.mapTransform)),d.alphaMap&&(m.alphaMap.value=d.alphaMap,t(d.alphaMap,m.alphaMapTransform)),d.bumpMap&&(m.bumpMap.value=d.bumpMap,t(d.bumpMap,m.bumpMapTransform),m.bumpScale.value=d.bumpScale,d.side===Jt&&(m.bumpScale.value*=-1)),d.normalMap&&(m.normalMap.value=d.normalMap,t(d.normalMap,m.normalMapTransform),m.normalScale.value.copy(d.normalScale),d.side===Jt&&m.normalScale.value.negate()),d.displacementMap&&(m.displacementMap.value=d.displacementMap,t(d.displacementMap,m.displacementMapTransform),m.displacementScale.value=d.displacementScale,m.displacementBias.value=d.displacementBias),d.emissiveMap&&(m.emissiveMap.value=d.emissiveMap,t(d.emissiveMap,m.emissiveMapTransform)),d.specularMap&&(m.specularMap.value=d.specularMap,t(d.specularMap,m.specularMapTransform)),d.alphaTest>0&&(m.alphaTest.value=d.alphaTest);let w=e.get(d),M=w.envMap,x=w.envMapRotation;M&&(m.envMap.value=M,or.copy(x),or.x*=-1,or.y*=-1,or.z*=-1,M.isCubeTexture&&M.isRenderTargetTexture===!1&&(or.y*=-1,or.z*=-1),m.envMapRotation.value.setFromMatrix4(mM.makeRotationFromEuler(or)),m.flipEnvMap.value=M.isCubeTexture&&M.isRenderTargetTexture===!1?-1:1,m.reflectivity.value=d.reflectivity,m.ior.value=d.ior,m.refractionRatio.value=d.refractionRatio),d.lightMap&&(m.lightMap.value=d.lightMap,m.lightMapIntensity.value=d.lightMapIntensity,t(d.lightMap,m.lightMapTransform)),d.aoMap&&(m.aoMap.value=d.aoMap,m.aoMapIntensity.value=d.aoMapIntensity,t(d.aoMap,m.aoMapTransform))}function a(m,d){m.diffuse.value.copy(d.color),m.opacity.value=d.opacity,d.map&&(m.map.value=d.map,t(d.map,m.mapTransform))}function o(m,d){m.dashSize.value=d.dashSize,m.totalSize.value=d.dashSize+d.gapSize,m.scale.value=d.scale}function c(m,d,w,M){m.diffuse.value.copy(d.color),m.opacity.value=d.opacity,m.size.value=d.size*w,m.scale.value=M*.5,d.map&&(m.map.value=d.map,t(d.map,m.uvTransform)),d.alphaMap&&(m.alphaMap.value=d.alphaMap,t(d.alphaMap,m.alphaMapTransform)),d.alphaTest>0&&(m.alphaTest.value=d.alphaTest)}function u(m,d){m.diffuse.value.copy(d.color),m.opacity.value=d.opacity,m.rotation.value=d.rotation,d.map&&(m.map.value=d.map,t(d.map,m.mapTransform)),d.alphaMap&&(m.alphaMap.value=d.alphaMap,t(d.alphaMap,m.alphaMapTransform)),d.alphaTest>0&&(m.alphaTest.value=d.alphaTest)}function l(m,d){m.specular.value.copy(d.specular),m.shininess.value=Math.max(d.shininess,1e-4)}function h(m,d){d.gradientMap&&(m.gradientMap.value=d.gradientMap)}function f(m,d){m.metalness.value=d.metalness,d.metalnessMap&&(m.metalnessMap.value=d.metalnessMap,t(d.metalnessMap,m.metalnessMapTransform)),m.roughness.value=d.roughness,d.roughnessMap&&(m.roughnessMap.value=d.roughnessMap,t(d.roughnessMap,m.roughnessMapTransform)),d.envMap&&(m.envMapIntensity.value=d.envMapIntensity)}function p(m,d,w){m.ior.value=d.ior,d.sheen>0&&(m.sheenColor.value.copy(d.sheenColor).multiplyScalar(d.sheen),m.sheenRoughness.value=d.sheenRoughness,d.sheenColorMap&&(m.sheenColorMap.value=d.sheenColorMap,t(d.sheenColorMap,m.sheenColorMapTransform)),d.sheenRoughnessMap&&(m.sheenRoughnessMap.value=d.sheenRoughnessMap,t(d.sheenRoughnessMap,m.sheenRoughnessMapTransform))),d.clearcoat>0&&(m.clearcoat.value=d.clearcoat,m.clearcoatRoughness.value=d.clearcoatRoughness,d.clearcoatMap&&(m.clearcoatMap.value=d.clearcoatMap,t(d.clearcoatMap,m.clearcoatMapTransform)),d.clearcoatRoughnessMap&&(m.clearcoatRoughnessMap.value=d.clearcoatRoughnessMap,t(d.clearcoatRoughnessMap,m.clearcoatRoughnessMapTransform)),d.clearcoatNormalMap&&(m.clearcoatNormalMap.value=d.clearcoatNormalMap,t(d.clearcoatNormalMap,m.clearcoatNormalMapTransform),m.clearcoatNormalScale.value.copy(d.clearcoatNormalScale),d.side===Jt&&m.clearcoatNormalScale.value.negate())),d.dispersion>0&&(m.dispersion.value=d.dispersion),d.iridescence>0&&(m.iridescence.value=d.iridescence,m.iridescenceIOR.value=d.iridescenceIOR,m.iridescenceThicknessMinimum.value=d.iridescenceThicknessRange[0],m.iridescenceThicknessMaximum.value=d.iridescenceThicknessRange[1],d.iridescenceMap&&(m.iridescenceMap.value=d.iridescenceMap,t(d.iridescenceMap,m.iridescenceMapTransform)),d.iridescenceThicknessMap&&(m.iridescenceThicknessMap.value=d.iridescenceThicknessMap,t(d.iridescenceThicknessMap,m.iridescenceThicknessMapTransform))),d.transmission>0&&(m.transmission.value=d.transmission,m.transmissionSamplerMap.value=w.texture,m.transmissionSamplerSize.value.set(w.width,w.height),d.transmissionMap&&(m.transmissionMap.value=d.transmissionMap,t(d.transmissionMap,m.transmissionMapTransform)),m.thickness.value=d.thickness,d.thicknessMap&&(m.thicknessMap.value=d.thicknessMap,t(d.thicknessMap,m.thicknessMapTransform)),m.attenuationDistance.value=d.attenuationDistance,m.attenuationColor.value.copy(d.attenuationColor)),d.anisotropy>0&&(m.anisotropyVector.value.set(d.anisotropy*Math.cos(d.anisotropyRotation),d.anisotropy*Math.sin(d.anisotropyRotation)),d.anisotropyMap&&(m.anisotropyMap.value=d.anisotropyMap,t(d.anisotropyMap,m.anisotropyMapTransform))),m.specularIntensity.value=d.specularIntensity,m.specularColor.value.copy(d.specularColor),d.specularColorMap&&(m.specularColorMap.value=d.specularColorMap,t(d.specularColorMap,m.specularColorMapTransform)),d.specularIntensityMap&&(m.specularIntensityMap.value=d.specularIntensityMap,t(d.specularIntensityMap,m.specularIntensityMapTransform))}function g(m,d){d.matcap&&(m.matcap.value=d.matcap)}function _(m,d){let w=e.get(d).light;m.referencePosition.value.setFromMatrixPosition(w.matrixWorld),m.nearDistance.value=w.shadow.camera.near,m.farDistance.value=w.shadow.camera.far}return{refreshFogUniforms:i,refreshMaterialUniforms:r}}function _M(n,e,t,i){let r={},s={},a=[],o=n.getParameter(n.MAX_UNIFORM_BUFFER_BINDINGS);function c(w,M){let x=M.program;i.uniformBlockBinding(w,x)}function u(w,M){let x=r[w.id];x===void 0&&(g(w),x=l(w),r[w.id]=x,w.addEventListener("dispose",m));let T=M.program;i.updateUBOMapping(w,T);let R=e.render.frame;s[w.id]!==R&&(f(w),s[w.id]=R)}function l(w){let M=h();w.__bindingPointIndex=M;let x=n.createBuffer(),T=w.__size,R=w.usage;return n.bindBuffer(n.UNIFORM_BUFFER,x),n.bufferData(n.UNIFORM_BUFFER,T,R),n.bindBuffer(n.UNIFORM_BUFFER,null),n.bindBufferBase(n.UNIFORM_BUFFER,M,x),x}function h(){for(let w=0;w<o;w++)if(a.indexOf(w)===-1)return a.push(w),w;return console.error("THREE.WebGLRenderer: Maximum number of simultaneously usable uniforms groups reached."),0}function f(w){let M=r[w.id],x=w.uniforms,T=w.__cache;n.bindBuffer(n.UNIFORM_BUFFER,M);for(let R=0,P=x.length;R<P;R++){let O=Array.isArray(x[R])?x[R]:[x[R]];for(let b=0,S=O.length;b<S;b++){let I=O[b];if(p(I,R,b,T)===!0){let k=I.__offset,V=Array.isArray(I.value)?I.value:[I.value],W=0;for(let j=0;j<V.length;j++){let G=V[j],ne=_(G);typeof G=="number"||typeof G=="boolean"?(I.__data[0]=G,n.bufferSubData(n.UNIFORM_BUFFER,k+W,I.__data)):G.isMatrix3?(I.__data[0]=G.elements[0],I.__data[1]=G.elements[1],I.__data[2]=G.elements[2],I.__data[3]=0,I.__data[4]=G.elements[3],I.__data[5]=G.elements[4],I.__data[6]=G.elements[5],I.__data[7]=0,I.__data[8]=G.elements[6],I.__data[9]=G.elements[7],I.__data[10]=G.elements[8],I.__data[11]=0):(G.toArray(I.__data,W),W+=ne.storage/Float32Array.BYTES_PER_ELEMENT)}n.bufferSubData(n.UNIFORM_BUFFER,k,I.__data)}}}n.bindBuffer(n.UNIFORM_BUFFER,null)}function p(w,M,x,T){let R=w.value,P=M+"_"+x;if(T[P]===void 0)return typeof R=="number"||typeof R=="boolean"?T[P]=R:T[P]=R.clone(),!0;{let O=T[P];if(typeof R=="number"||typeof R=="boolean"){if(O!==R)return T[P]=R,!0}else if(O.equals(R)===!1)return O.copy(R),!0}return!1}function g(w){let M=w.uniforms,x=0,T=16;for(let P=0,O=M.length;P<O;P++){let b=Array.isArray(M[P])?M[P]:[M[P]];for(let S=0,I=b.length;S<I;S++){let k=b[S],V=Array.isArray(k.value)?k.value:[k.value];for(let W=0,j=V.length;W<j;W++){let G=V[W],ne=_(G),z=x%T,ae=z%ne.boundary,ue=z+ae;x+=ae,ue!==0&&T-ue<ne.storage&&(x+=T-ue),k.__data=new Float32Array(ne.storage/Float32Array.BYTES_PER_ELEMENT),k.__offset=x,x+=ne.storage}}}let R=x%T;return R>0&&(x+=T-R),w.__size=x,w.__cache={},this}function _(w){let M={boundary:0,storage:0};return typeof w=="number"||typeof w=="boolean"?(M.boundary=4,M.storage=4):w.isVector2?(M.boundary=8,M.storage=8):w.isVector3||w.isColor?(M.boundary=16,M.storage=12):w.isVector4?(M.boundary=16,M.storage=16):w.isMatrix3?(M.boundary=48,M.storage=48):w.isMatrix4?(M.boundary=64,M.storage=64):w.isTexture?console.warn("THREE.WebGLRenderer: Texture samplers can not be part of an uniforms group."):console.warn("THREE.WebGLRenderer: Unsupported uniform value type.",w),M}function m(w){let M=w.target;M.removeEventListener("dispose",m);let x=a.indexOf(M.__bindingPointIndex);a.splice(x,1),n.deleteBuffer(r[M.id]),delete r[M.id],delete s[M.id]}function d(){for(let w in r)n.deleteBuffer(r[w]);a=[],r={},s={}}return{bind:c,update:u,dispose:d}}var Cc=class{constructor(e={}){let{canvas:t=vp(),context:i=null,depth:r=!0,stencil:s=!1,alpha:a=!1,antialias:o=!1,premultipliedAlpha:c=!0,preserveDrawingBuffer:u=!1,powerPreference:l="default",failIfMajorPerformanceCaveat:h=!1,reversedDepthBuffer:f=!1}=e;this.isWebGLRenderer=!0;let p;if(i!==null){if(typeof WebGLRenderingContext<"u"&&i instanceof WebGLRenderingContext)throw new Error("THREE.WebGLRenderer: WebGL 1 is not supported since r163.");p=i.getContextAttributes().alpha}else p=a;let g=new Uint32Array(4),_=new Int32Array(4),m=null,d=null,w=[],M=[];this.domElement=t,this.debug={checkShaderErrors:!0,onShaderError:null},this.autoClear=!0,this.autoClearColor=!0,this.autoClearDepth=!0,this.autoClearStencil=!0,this.sortObjects=!0,this.clippingPlanes=[],this.localClippingEnabled=!1,this.toneMapping=hi,this.toneMappingExposure=1,this.transmissionResolutionScale=1;let x=this,T=!1;this._outputColorSpace=Vt;let R=0,P=0,O=null,b=-1,S=null,I=new Tt,k=new Tt,V=null,W=new Qe(0),j=0,G=t.width,ne=t.height,z=1,ae=null,ue=null,we=new Tt(0,0,G,ne),je=new Tt(0,0,G,ne),ft=!1,xt=new Ws,rt=!1,q=!1,Z=new _t,fe=new L,Ne=new Tt,Ee={background:null,fog:null,environment:null,overrideMaterial:null,isScene:!0},Je=!1;function Bt(){return O===null?z:1}let A=i;function St(v,D){return t.getContext(v,D)}try{let v={alpha:!0,depth:r,stencil:s,antialias:o,premultipliedAlpha:c,preserveDrawingBuffer:u,powerPreference:l,failIfMajorPerformanceCaveat:h};if("setAttribute"in t&&t.setAttribute("data-engine",`three.js r${"180"}`),t.addEventListener("webglcontextlost",re,!1),t.addEventListener("webglcontextrestored",de,!1),t.addEventListener("webglcontextcreationerror",K,!1),A===null){let D="webgl2";if(A=St(D,v),A===null)throw St(D)?new Error("Error creating WebGL context with your selected attributes."):new Error("Error creating WebGL context.")}}catch(v){throw console.error("THREE.WebGLRenderer: "+v.message),v}let Oe,Pe,ge,bt,_e,$e,Nt,At,E,y,U,H,Y,$,Me,ie,ye,Se,Q,le,Ce,be,oe,ke;function C(){Oe=new US(A),Oe.init(),be=new dM(A,Oe),Pe=new RS(A,Oe,e,be),ge=new uM(A,Oe),Pe.reversedDepthBuffer&&f&&ge.buffers.depth.setReversed(!0),bt=new BS(A),_e=new Kb,$e=new hM(A,Oe,ge,_e,Pe,be,bt),Nt=new PS(x),At=new LS(x),E=new Gy(A),oe=new TS(A,E),y=new OS(A,E,bt,oe),U=new zS(A,y,E,bt),Q=new kS(A,Pe,$e),ie=new CS(_e),H=new Zb(x,Nt,At,Oe,Pe,oe,ie),Y=new gM(x,_e),$=new Qb,Me=new sM(Oe),Se=new wS(x,Nt,At,ge,U,p,c),ye=new cM(x,U,Pe),ke=new _M(A,bt,Pe,ge),le=new AS(A,Oe,bt),Ce=new FS(A,Oe,bt),bt.programs=H.programs,x.capabilities=Pe,x.extensions=Oe,x.properties=_e,x.renderLists=$,x.shadowMap=ye,x.state=ge,x.info=bt}C();let ee=new Uu(x,A);this.xr=ee,this.getContext=function(){return A},this.getContextAttributes=function(){return A.getContextAttributes()},this.forceContextLoss=function(){let v=Oe.get("WEBGL_lose_context");v&&v.loseContext()},this.forceContextRestore=function(){let v=Oe.get("WEBGL_lose_context");v&&v.restoreContext()},this.getPixelRatio=function(){return z},this.setPixelRatio=function(v){v!==void 0&&(z=v,this.setSize(G,ne,!1))},this.getSize=function(v){return v.set(G,ne)},this.setSize=function(v,D,F=!0){if(ee.isPresenting){console.warn("THREE.WebGLRenderer: Can't change size while VR device is presenting.");return}G=v,ne=D,t.width=Math.floor(v*z),t.height=Math.floor(D*z),F===!0&&(t.style.width=v+"px",t.style.height=D+"px"),this.setViewport(0,0,v,D)},this.getDrawingBufferSize=function(v){return v.set(G*z,ne*z).floor()},this.setDrawingBufferSize=function(v,D,F){G=v,ne=D,z=F,t.width=Math.floor(v*F),t.height=Math.floor(D*F),this.setViewport(0,0,v,D)},this.getCurrentViewport=function(v){return v.copy(I)},this.getViewport=function(v){return v.copy(we)},this.setViewport=function(v,D,F,B){v.isVector4?we.set(v.x,v.y,v.z,v.w):we.set(v,D,F,B),ge.viewport(I.copy(we).multiplyScalar(z).round())},this.getScissor=function(v){return v.copy(je)},this.setScissor=function(v,D,F,B){v.isVector4?je.set(v.x,v.y,v.z,v.w):je.set(v,D,F,B),ge.scissor(k.copy(je).multiplyScalar(z).round())},this.getScissorTest=function(){return ft},this.setScissorTest=function(v){ge.setScissorTest(ft=v)},this.setOpaqueSort=function(v){ae=v},this.setTransparentSort=function(v){ue=v},this.getClearColor=function(v){return v.copy(Se.getClearColor())},this.setClearColor=function(){Se.setClearColor(...arguments)},this.getClearAlpha=function(){return Se.getClearAlpha()},this.setClearAlpha=function(){Se.setClearAlpha(...arguments)},this.clear=function(v=!0,D=!0,F=!0){let B=0;if(v){let N=!1;if(O!==null){let J=O.texture.format;N=J===Zo||J===Yo||J===jo}if(N){let J=O.texture.type,ce=J===Zn||J===Ni||J===Jr||J===es||J===Wo||J===qo,pe=Se.getClearColor(),he=Se.getClearAlpha(),Re=pe.r,Ie=pe.g,Te=pe.b;ce?(g[0]=Re,g[1]=Ie,g[2]=Te,g[3]=he,A.clearBufferuiv(A.COLOR,0,g)):(_[0]=Re,_[1]=Ie,_[2]=Te,_[3]=he,A.clearBufferiv(A.COLOR,0,_))}else B|=A.COLOR_BUFFER_BIT}D&&(B|=A.DEPTH_BUFFER_BIT),F&&(B|=A.STENCIL_BUFFER_BIT,this.state.buffers.stencil.setMask(4294967295)),A.clear(B)},this.clearColor=function(){this.clear(!0,!1,!1)},this.clearDepth=function(){this.clear(!1,!0,!1)},this.clearStencil=function(){this.clear(!1,!1,!0)},this.dispose=function(){t.removeEventListener("webglcontextlost",re,!1),t.removeEventListener("webglcontextrestored",de,!1),t.removeEventListener("webglcontextcreationerror",K,!1),Se.dispose(),$.dispose(),Me.dispose(),_e.dispose(),Nt.dispose(),At.dispose(),U.dispose(),oe.dispose(),ke.dispose(),H.dispose(),ee.dispose(),ee.removeEventListener("sessionstart",Hn),ee.removeEventListener("sessionend",tf),Hi.stop()};function re(v){v.preventDefault(),console.log("THREE.WebGLRenderer: Context Lost."),T=!0}function de(){console.log("THREE.WebGLRenderer: Context Restored."),T=!1;let v=bt.autoReset,D=ye.enabled,F=ye.autoUpdate,B=ye.needsUpdate,N=ye.type;C(),bt.autoReset=v,ye.enabled=D,ye.autoUpdate=F,ye.needsUpdate=B,ye.type=N}function K(v){console.error("THREE.WebGLRenderer: A WebGL context could not be created. Reason: ",v.statusMessage)}function X(v){let D=v.target;D.removeEventListener("dispose",X),me(D)}function me(v){Le(v),_e.remove(v)}function Le(v){let D=_e.get(v).programs;D!==void 0&&(D.forEach(function(F){H.releaseProgram(F)}),v.isShaderMaterial&&H.releaseShaderCache(v))}this.renderBufferDirect=function(v,D,F,B,N,J){D===null&&(D=Ee);let ce=N.isMesh&&N.matrixWorld.determinant()<0,pe=O0(v,D,F,B,N);ge.setMaterial(B,ce);let he=F.index,Re=1;if(B.wireframe===!0){if(he=y.getWireframeAttribute(F),he===void 0)return;Re=2}let Ie=F.drawRange,Te=F.attributes.position,Ze=Ie.start*Re,ct=(Ie.start+Ie.count)*Re;J!==null&&(Ze=Math.max(Ze,J.start*Re),ct=Math.min(ct,(J.start+J.count)*Re)),he!==null?(Ze=Math.max(Ze,0),ct=Math.min(ct,he.count)):Te!=null&&(Ze=Math.max(Ze,0),ct=Math.min(ct,Te.count));let wt=ct-Ze;if(wt<0||wt===1/0)return;oe.setup(N,B,pe,F,he);let mt,ht=le;if(he!==null&&(mt=E.get(he),ht=Ce,ht.setIndex(mt)),N.isMesh)B.wireframe===!0?(ge.setLineWidth(B.wireframeLinewidth*Bt()),ht.setMode(A.LINES)):ht.setMode(A.TRIANGLES);else if(N.isLine){let Ae=B.linewidth;Ae===void 0&&(Ae=1),ge.setLineWidth(Ae*Bt()),N.isLineSegments?ht.setMode(A.LINES):N.isLineLoop?ht.setMode(A.LINE_LOOP):ht.setMode(A.LINE_STRIP)}else N.isPoints?ht.setMode(A.POINTS):N.isSprite&&ht.setMode(A.TRIANGLES);if(N.isBatchedMesh)if(N._multiDrawInstances!==null)qr("THREE.WebGLRenderer: renderMultiDrawInstances has been deprecated and will be removed in r184. Append to renderMultiDraw arguments and use indirection."),ht.renderMultiDrawInstances(N._multiDrawStarts,N._multiDrawCounts,N._multiDrawCount,N._multiDrawInstances);else if(Oe.get("WEBGL_multi_draw"))ht.renderMultiDraw(N._multiDrawStarts,N._multiDrawCounts,N._multiDrawCount);else{let Ae=N._multiDrawStarts,Mt=N._multiDrawCounts,it=N._multiDrawCount,mn=he?E.get(he).bytesPerElement:1,Rr=_e.get(B).currentProgram.getUniforms();for(let gn=0;gn<it;gn++)Rr.setValue(A,"_gl_DrawID",gn),ht.render(Ae[gn]/mn,Mt[gn])}else if(N.isInstancedMesh)ht.renderInstances(Ze,wt,N.count);else if(F.isInstancedBufferGeometry){let Ae=F._maxInstanceCount!==void 0?F._maxInstanceCount:1/0,Mt=Math.min(F.instanceCount,Ae);ht.renderInstances(Ze,wt,Mt)}else ht.render(Ze,wt)};function pt(v,D,F){v.transparent===!0&&v.side===Yn&&v.forceSinglePass===!1?(v.side=Jt,v.needsUpdate=!0,$a(v,D,F),v.side=ci,v.needsUpdate=!0,$a(v,D,F),v.side=Yn):$a(v,D,F)}this.compile=function(v,D,F=null){F===null&&(F=v),d=Me.get(F),d.init(D),M.push(d),F.traverseVisible(function(N){N.isLight&&N.layers.test(D.layers)&&(d.pushLight(N),N.castShadow&&d.pushShadow(N))}),v!==F&&v.traverseVisible(function(N){N.isLight&&N.layers.test(D.layers)&&(d.pushLight(N),N.castShadow&&d.pushShadow(N))}),d.setupLights();let B=new Set;return v.traverse(function(N){if(!(N.isMesh||N.isPoints||N.isLine||N.isSprite))return;let J=N.material;if(J)if(Array.isArray(J))for(let ce=0;ce<J.length;ce++){let pe=J[ce];pt(pe,F,N),B.add(pe)}else pt(J,F,N),B.add(J)}),d=M.pop(),B},this.compileAsync=function(v,D,F=null){let B=this.compile(v,D,F);return new Promise(N=>{function J(){if(B.forEach(function(ce){_e.get(ce).currentProgram.isReady()&&B.delete(ce)}),B.size===0){N(v);return}setTimeout(J,10)}Oe.get("KHR_parallel_shader_compile")!==null?J():setTimeout(J,10)})};let st=null;function ti(v){st&&st(v)}function Hn(){Hi.stop()}function tf(){Hi.start()}let Hi=new Xp;Hi.setAnimationLoop(ti),typeof self<"u"&&Hi.setContext(self),this.setAnimationLoop=function(v){st=v,ee.setAnimationLoop(v),v===null?Hi.stop():Hi.start()},ee.addEventListener("sessionstart",Hn),ee.addEventListener("sessionend",tf),this.render=function(v,D){if(D!==void 0&&D.isCamera!==!0){console.error("THREE.WebGLRenderer.render: camera is not an instance of THREE.Camera.");return}if(T===!0)return;if(v.matrixWorldAutoUpdate===!0&&v.updateMatrixWorld(),D.parent===null&&D.matrixWorldAutoUpdate===!0&&D.updateMatrixWorld(),ee.enabled===!0&&ee.isPresenting===!0&&(ee.cameraAutoUpdate===!0&&ee.updateCamera(D),D=ee.getCamera()),v.isScene===!0&&v.onBeforeRender(x,v,D,O),d=Me.get(v,M.length),d.init(D),M.push(d),Z.multiplyMatrices(D.projectionMatrix,D.matrixWorldInverse),xt.setFromProjectionMatrix(Z,Un,D.reversedDepth),q=this.localClippingEnabled,rt=ie.init(this.clippingPlanes,q),m=$.get(v,w.length),m.init(),w.push(m),ee.enabled===!0&&ee.isPresenting===!0){let J=x.xr.getDepthSensingMesh();J!==null&&El(J,D,-1/0,x.sortObjects)}El(v,D,0,x.sortObjects),m.finish(),x.sortObjects===!0&&m.sort(ae,ue),Je=ee.enabled===!1||ee.isPresenting===!1||ee.hasDepthSensing()===!1,Je&&Se.addToRenderList(m,v),this.info.render.frame++,rt===!0&&ie.beginShadows();let F=d.state.shadowsArray;ye.render(F,v,D),rt===!0&&ie.endShadows(),this.info.autoReset===!0&&this.info.reset();let B=m.opaque,N=m.transmissive;if(d.setupLights(),D.isArrayCamera){let J=D.cameras;if(N.length>0)for(let ce=0,pe=J.length;ce<pe;ce++){let he=J[ce];rf(B,N,v,he)}Je&&Se.render(v);for(let ce=0,pe=J.length;ce<pe;ce++){let he=J[ce];nf(m,v,he,he.viewport)}}else N.length>0&&rf(B,N,v,D),Je&&Se.render(v),nf(m,v,D);O!==null&&P===0&&($e.updateMultisampleRenderTarget(O),$e.updateRenderTargetMipmap(O)),v.isScene===!0&&v.onAfterRender(x,v,D),oe.resetDefaultState(),b=-1,S=null,M.pop(),M.length>0?(d=M[M.length-1],rt===!0&&ie.setGlobalState(x.clippingPlanes,d.state.camera)):d=null,w.pop(),w.length>0?m=w[w.length-1]:m=null};function El(v,D,F,B){if(v.visible===!1)return;if(v.layers.test(D.layers)){if(v.isGroup)F=v.renderOrder;else if(v.isLOD)v.autoUpdate===!0&&v.update(D);else if(v.isLight)d.pushLight(v),v.castShadow&&d.pushShadow(v);else if(v.isSprite){if(!v.frustumCulled||xt.intersectsSprite(v)){B&&Ne.setFromMatrixPosition(v.matrixWorld).applyMatrix4(Z);let ce=U.update(v),pe=v.material;pe.visible&&m.push(v,ce,pe,F,Ne.z,null)}}else if((v.isMesh||v.isLine||v.isPoints)&&(!v.frustumCulled||xt.intersectsObject(v))){let ce=U.update(v),pe=v.material;if(B&&(v.boundingSphere!==void 0?(v.boundingSphere===null&&v.computeBoundingSphere(),Ne.copy(v.boundingSphere.center)):(ce.boundingSphere===null&&ce.computeBoundingSphere(),Ne.copy(ce.boundingSphere.center)),Ne.applyMatrix4(v.matrixWorld).applyMatrix4(Z)),Array.isArray(pe)){let he=ce.groups;for(let Re=0,Ie=he.length;Re<Ie;Re++){let Te=he[Re],Ze=pe[Te.materialIndex];Ze&&Ze.visible&&m.push(v,ce,Ze,F,Ne.z,Te)}}else pe.visible&&m.push(v,ce,pe,F,Ne.z,null)}}let J=v.children;for(let ce=0,pe=J.length;ce<pe;ce++)El(J[ce],D,F,B)}function nf(v,D,F,B){let N=v.opaque,J=v.transmissive,ce=v.transparent;d.setupLightsView(F),rt===!0&&ie.setGlobalState(x.clippingPlanes,F),B&&ge.viewport(I.copy(B)),N.length>0&&Va(N,D,F),J.length>0&&Va(J,D,F),ce.length>0&&Va(ce,D,F),ge.buffers.depth.setTest(!0),ge.buffers.depth.setMask(!0),ge.buffers.color.setMask(!0),ge.setPolygonOffset(!1)}function rf(v,D,F,B){if((F.isScene===!0?F.overrideMaterial:null)!==null)return;d.state.transmissionRenderTarget[B.id]===void 0&&(d.state.transmissionRenderTarget[B.id]=new Wn(1,1,{generateMipmaps:!0,type:Oe.has("EXT_color_buffer_half_float")||Oe.has("EXT_color_buffer_float")?Qr:Zn,minFilter:Di,samples:4,stencilBuffer:s,resolveDepthBuffer:!1,resolveStencilBuffer:!1,colorSpace:tt.workingColorSpace}));let J=d.state.transmissionRenderTarget[B.id],ce=B.viewport||I;J.setSize(ce.z*x.transmissionResolutionScale,ce.w*x.transmissionResolutionScale);let pe=x.getRenderTarget(),he=x.getActiveCubeFace(),Re=x.getActiveMipmapLevel();x.setRenderTarget(J),x.getClearColor(W),j=x.getClearAlpha(),j<1&&x.setClearColor(16777215,.5),x.clear(),Je&&Se.render(F);let Ie=x.toneMapping;x.toneMapping=hi;let Te=B.viewport;if(B.viewport!==void 0&&(B.viewport=void 0),d.setupLightsView(B),rt===!0&&ie.setGlobalState(x.clippingPlanes,B),Va(v,F,B),$e.updateMultisampleRenderTarget(J),$e.updateRenderTargetMipmap(J),Oe.has("WEBGL_multisampled_render_to_texture")===!1){let Ze=!1;for(let ct=0,wt=D.length;ct<wt;ct++){let mt=D[ct],ht=mt.object,Ae=mt.geometry,Mt=mt.material,it=mt.group;if(Mt.side===Yn&&ht.layers.test(B.layers)){let mn=Mt.side;Mt.side=Jt,Mt.needsUpdate=!0,sf(ht,F,B,Ae,Mt,it),Mt.side=mn,Mt.needsUpdate=!0,Ze=!0}}Ze===!0&&($e.updateMultisampleRenderTarget(J),$e.updateRenderTargetMipmap(J))}x.setRenderTarget(pe,he,Re),x.setClearColor(W,j),Te!==void 0&&(B.viewport=Te),x.toneMapping=Ie}function Va(v,D,F){let B=D.isScene===!0?D.overrideMaterial:null;for(let N=0,J=v.length;N<J;N++){let ce=v[N],pe=ce.object,he=ce.geometry,Re=ce.group,Ie=ce.material;Ie.allowOverride===!0&&B!==null&&(Ie=B),pe.layers.test(F.layers)&&sf(pe,D,F,he,Ie,Re)}}function sf(v,D,F,B,N,J){v.onBeforeRender(x,D,F,B,N,J),v.modelViewMatrix.multiplyMatrices(F.matrixWorldInverse,v.matrixWorld),v.normalMatrix.getNormalMatrix(v.modelViewMatrix),N.onBeforeRender(x,D,F,B,v,J),N.transparent===!0&&N.side===Yn&&N.forceSinglePass===!1?(N.side=Jt,N.needsUpdate=!0,x.renderBufferDirect(F,D,B,N,v,J),N.side=ci,N.needsUpdate=!0,x.renderBufferDirect(F,D,B,N,v,J),N.side=Yn):x.renderBufferDirect(F,D,B,N,v,J),v.onAfterRender(x,D,F,B,N,J)}function $a(v,D,F){D.isScene!==!0&&(D=Ee);let B=_e.get(v),N=d.state.lights,J=d.state.shadowsArray,ce=N.state.version,pe=H.getParameters(v,N.state,J,D,F),he=H.getProgramCacheKey(pe),Re=B.programs;B.environment=v.isMeshStandardMaterial?D.environment:null,B.fog=D.fog,B.envMap=(v.isMeshStandardMaterial?At:Nt).get(v.envMap||B.environment),B.envMapRotation=B.environment!==null&&v.envMap===null?D.environmentRotation:v.envMapRotation,Re===void 0&&(v.addEventListener("dispose",X),Re=new Map,B.programs=Re);let Ie=Re.get(he);if(Ie!==void 0){if(B.currentProgram===Ie&&B.lightsStateVersion===ce)return of(v,pe),Ie}else pe.uniforms=H.getUniforms(v),v.onBeforeCompile(pe,x),Ie=H.acquireProgram(pe,he),Re.set(he,Ie),B.uniforms=pe.uniforms;let Te=B.uniforms;return(!v.isShaderMaterial&&!v.isRawShaderMaterial||v.clipping===!0)&&(Te.clippingPlanes=ie.uniform),of(v,pe),B.needsLights=B0(v),B.lightsStateVersion=ce,B.needsLights&&(Te.ambientLightColor.value=N.state.ambient,Te.lightProbe.value=N.state.probe,Te.directionalLights.value=N.state.directional,Te.directionalLightShadows.value=N.state.directionalShadow,Te.spotLights.value=N.state.spot,Te.spotLightShadows.value=N.state.spotShadow,Te.rectAreaLights.value=N.state.rectArea,Te.ltc_1.value=N.state.rectAreaLTC1,Te.ltc_2.value=N.state.rectAreaLTC2,Te.pointLights.value=N.state.point,Te.pointLightShadows.value=N.state.pointShadow,Te.hemisphereLights.value=N.state.hemi,Te.directionalShadowMap.value=N.state.directionalShadowMap,Te.directionalShadowMatrix.value=N.state.directionalShadowMatrix,Te.spotShadowMap.value=N.state.spotShadowMap,Te.spotLightMatrix.value=N.state.spotLightMatrix,Te.spotLightMap.value=N.state.spotLightMap,Te.pointShadowMap.value=N.state.pointShadowMap,Te.pointShadowMatrix.value=N.state.pointShadowMatrix),B.currentProgram=Ie,B.uniformsList=null,Ie}function af(v){if(v.uniformsList===null){let D=v.currentProgram.getUniforms();v.uniformsList=ss.seqWithValue(D.seq,v.uniforms)}return v.uniformsList}function of(v,D){let F=_e.get(v);F.outputColorSpace=D.outputColorSpace,F.batching=D.batching,F.batchingColor=D.batchingColor,F.instancing=D.instancing,F.instancingColor=D.instancingColor,F.instancingMorph=D.instancingMorph,F.skinning=D.skinning,F.morphTargets=D.morphTargets,F.morphNormals=D.morphNormals,F.morphColors=D.morphColors,F.morphTargetsCount=D.morphTargetsCount,F.numClippingPlanes=D.numClippingPlanes,F.numIntersection=D.numClipIntersection,F.vertexAlphas=D.vertexAlphas,F.vertexTangents=D.vertexTangents,F.toneMapping=D.toneMapping}function O0(v,D,F,B,N){D.isScene!==!0&&(D=Ee),$e.resetTextureUnits();let J=D.fog,ce=B.isMeshStandardMaterial?D.environment:null,pe=O===null?x.outputColorSpace:O.isXRRenderTarget===!0?O.texture.colorSpace:Qi,he=(B.isMeshStandardMaterial?At:Nt).get(B.envMap||ce),Re=B.vertexColors===!0&&!!F.attributes.color&&F.attributes.color.itemSize===4,Ie=!!F.attributes.tangent&&(!!B.normalMap||B.anisotropy>0),Te=!!F.morphAttributes.position,Ze=!!F.morphAttributes.normal,ct=!!F.morphAttributes.color,wt=hi;B.toneMapped&&(O===null||O.isXRRenderTarget===!0)&&(wt=x.toneMapping);let mt=F.morphAttributes.position||F.morphAttributes.normal||F.morphAttributes.color,ht=mt!==void 0?mt.length:0,Ae=_e.get(B),Mt=d.state.lights;if(rt===!0&&(q===!0||v!==S)){let Zt=v===S&&B.id===b;ie.setState(B,v,Zt)}let it=!1;B.version===Ae.__version?(Ae.needsLights&&Ae.lightsStateVersion!==Mt.state.version||Ae.outputColorSpace!==pe||N.isBatchedMesh&&Ae.batching===!1||!N.isBatchedMesh&&Ae.batching===!0||N.isBatchedMesh&&Ae.batchingColor===!0&&N.colorTexture===null||N.isBatchedMesh&&Ae.batchingColor===!1&&N.colorTexture!==null||N.isInstancedMesh&&Ae.instancing===!1||!N.isInstancedMesh&&Ae.instancing===!0||N.isSkinnedMesh&&Ae.skinning===!1||!N.isSkinnedMesh&&Ae.skinning===!0||N.isInstancedMesh&&Ae.instancingColor===!0&&N.instanceColor===null||N.isInstancedMesh&&Ae.instancingColor===!1&&N.instanceColor!==null||N.isInstancedMesh&&Ae.instancingMorph===!0&&N.morphTexture===null||N.isInstancedMesh&&Ae.instancingMorph===!1&&N.morphTexture!==null||Ae.envMap!==he||B.fog===!0&&Ae.fog!==J||Ae.numClippingPlanes!==void 0&&(Ae.numClippingPlanes!==ie.numPlanes||Ae.numIntersection!==ie.numIntersection)||Ae.vertexAlphas!==Re||Ae.vertexTangents!==Ie||Ae.morphTargets!==Te||Ae.morphNormals!==Ze||Ae.morphColors!==ct||Ae.toneMapping!==wt||Ae.morphTargetsCount!==ht)&&(it=!0):(it=!0,Ae.__version=B.version);let mn=Ae.currentProgram;it===!0&&(mn=$a(B,D,N));let Rr=!1,gn=!1,Es=!1,Et=mn.getUniforms(),bn=Ae.uniforms;if(ge.useProgram(mn.program)&&(Rr=!0,gn=!0,Es=!0),B.id!==b&&(b=B.id,gn=!0),Rr||S!==v){ge.buffers.depth.getReversed()&&v.reversedDepth!==!0&&(v._reversedDepth=!0,v.updateProjectionMatrix()),Et.setValue(A,"projectionMatrix",v.projectionMatrix),Et.setValue(A,"viewMatrix",v.matrixWorldInverse);let nn=Et.map.cameraPosition;nn!==void 0&&nn.setValue(A,fe.setFromMatrixPosition(v.matrixWorld)),Pe.logarithmicDepthBuffer&&Et.setValue(A,"logDepthBufFC",2/(Math.log(v.far+1)/Math.LN2)),(B.isMeshPhongMaterial||B.isMeshToonMaterial||B.isMeshLambertMaterial||B.isMeshBasicMaterial||B.isMeshStandardMaterial||B.isShaderMaterial)&&Et.setValue(A,"isOrthographic",v.isOrthographicCamera===!0),S!==v&&(S=v,gn=!0,Es=!0)}if(N.isSkinnedMesh){Et.setOptional(A,N,"bindMatrix"),Et.setOptional(A,N,"bindMatrixInverse");let Zt=N.skeleton;Zt&&(Zt.boneTexture===null&&Zt.computeBoneTexture(),Et.setValue(A,"boneTexture",Zt.boneTexture,$e))}N.isBatchedMesh&&(Et.setOptional(A,N,"batchingTexture"),Et.setValue(A,"batchingTexture",N._matricesTexture,$e),Et.setOptional(A,N,"batchingIdTexture"),Et.setValue(A,"batchingIdTexture",N._indirectTexture,$e),Et.setOptional(A,N,"batchingColorTexture"),N._colorsTexture!==null&&Et.setValue(A,"batchingColorTexture",N._colorsTexture,$e));let Mn=F.morphAttributes;if((Mn.position!==void 0||Mn.normal!==void 0||Mn.color!==void 0)&&Q.update(N,F,mn),(gn||Ae.receiveShadow!==N.receiveShadow)&&(Ae.receiveShadow=N.receiveShadow,Et.setValue(A,"receiveShadow",N.receiveShadow)),B.isMeshGouraudMaterial&&B.envMap!==null&&(bn.envMap.value=he,bn.flipEnvMap.value=he.isCubeTexture&&he.isRenderTargetTexture===!1?-1:1),B.isMeshStandardMaterial&&B.envMap===null&&D.environment!==null&&(bn.envMapIntensity.value=D.environmentIntensity),gn&&(Et.setValue(A,"toneMappingExposure",x.toneMappingExposure),Ae.needsLights&&F0(bn,Es),J&&B.fog===!0&&Y.refreshFogUniforms(bn,J),Y.refreshMaterialUniforms(bn,B,z,ne,d.state.transmissionRenderTarget[v.id]),ss.upload(A,af(Ae),bn,$e)),B.isShaderMaterial&&B.uniformsNeedUpdate===!0&&(ss.upload(A,af(Ae),bn,$e),B.uniformsNeedUpdate=!1),B.isSpriteMaterial&&Et.setValue(A,"center",N.center),Et.setValue(A,"modelViewMatrix",N.modelViewMatrix),Et.setValue(A,"normalMatrix",N.normalMatrix),Et.setValue(A,"modelMatrix",N.matrixWorld),B.isShaderMaterial||B.isRawShaderMaterial){let Zt=B.uniformsGroups;for(let nn=0,wl=Zt.length;nn<wl;nn++){let Gi=Zt[nn];ke.update(Gi,mn),ke.bind(Gi,mn)}}return mn}function F0(v,D){v.ambientLightColor.needsUpdate=D,v.lightProbe.needsUpdate=D,v.directionalLights.needsUpdate=D,v.directionalLightShadows.needsUpdate=D,v.pointLights.needsUpdate=D,v.pointLightShadows.needsUpdate=D,v.spotLights.needsUpdate=D,v.spotLightShadows.needsUpdate=D,v.rectAreaLights.needsUpdate=D,v.hemisphereLights.needsUpdate=D}function B0(v){return v.isMeshLambertMaterial||v.isMeshToonMaterial||v.isMeshPhongMaterial||v.isMeshStandardMaterial||v.isShadowMaterial||v.isShaderMaterial&&v.lights===!0}this.getActiveCubeFace=function(){return R},this.getActiveMipmapLevel=function(){return P},this.getRenderTarget=function(){return O},this.setRenderTargetTextures=function(v,D,F){let B=_e.get(v);B.__autoAllocateDepthBuffer=v.resolveDepthBuffer===!1,B.__autoAllocateDepthBuffer===!1&&(B.__useRenderToTexture=!1),_e.get(v.texture).__webglTexture=D,_e.get(v.depthTexture).__webglTexture=B.__autoAllocateDepthBuffer?void 0:F,B.__hasExternalTextures=!0},this.setRenderTargetFramebuffer=function(v,D){let F=_e.get(v);F.__webglFramebuffer=D,F.__useDefaultFramebuffer=D===void 0};let k0=A.createFramebuffer();this.setRenderTarget=function(v,D=0,F=0){O=v,R=D,P=F;let B=!0,N=null,J=!1,ce=!1;if(v){let he=_e.get(v);if(he.__useDefaultFramebuffer!==void 0)ge.bindFramebuffer(A.FRAMEBUFFER,null),B=!1;else if(he.__webglFramebuffer===void 0)$e.setupRenderTarget(v);else if(he.__hasExternalTextures)$e.rebindTextures(v,_e.get(v.texture).__webglTexture,_e.get(v.depthTexture).__webglTexture);else if(v.depthBuffer){let Te=v.depthTexture;if(he.__boundDepthTexture!==Te){if(Te!==null&&_e.has(Te)&&(v.width!==Te.image.width||v.height!==Te.image.height))throw new Error("WebGLRenderTarget: Attached DepthTexture is initialized to the incorrect size.");$e.setupDepthRenderbuffer(v)}}let Re=v.texture;(Re.isData3DTexture||Re.isDataArrayTexture||Re.isCompressedArrayTexture)&&(ce=!0);let Ie=_e.get(v).__webglFramebuffer;v.isWebGLCubeRenderTarget?(Array.isArray(Ie[D])?N=Ie[D][F]:N=Ie[D],J=!0):v.samples>0&&$e.useMultisampledRTT(v)===!1?N=_e.get(v).__webglMultisampledFramebuffer:Array.isArray(Ie)?N=Ie[F]:N=Ie,I.copy(v.viewport),k.copy(v.scissor),V=v.scissorTest}else I.copy(we).multiplyScalar(z).floor(),k.copy(je).multiplyScalar(z).floor(),V=ft;if(F!==0&&(N=k0),ge.bindFramebuffer(A.FRAMEBUFFER,N)&&B&&ge.drawBuffers(v,N),ge.viewport(I),ge.scissor(k),ge.setScissorTest(V),J){let he=_e.get(v.texture);A.framebufferTexture2D(A.FRAMEBUFFER,A.COLOR_ATTACHMENT0,A.TEXTURE_CUBE_MAP_POSITIVE_X+D,he.__webglTexture,F)}else if(ce){let he=D;for(let Re=0;Re<v.textures.length;Re++){let Ie=_e.get(v.textures[Re]);A.framebufferTextureLayer(A.FRAMEBUFFER,A.COLOR_ATTACHMENT0+Re,Ie.__webglTexture,F,he)}}else if(v!==null&&F!==0){let he=_e.get(v.texture);A.framebufferTexture2D(A.FRAMEBUFFER,A.COLOR_ATTACHMENT0,A.TEXTURE_2D,he.__webglTexture,F)}b=-1},this.readRenderTargetPixels=function(v,D,F,B,N,J,ce,pe=0){if(!(v&&v.isWebGLRenderTarget)){console.error("THREE.WebGLRenderer.readRenderTargetPixels: renderTarget is not THREE.WebGLRenderTarget.");return}let he=_e.get(v).__webglFramebuffer;if(v.isWebGLCubeRenderTarget&&ce!==void 0&&(he=he[ce]),he){ge.bindFramebuffer(A.FRAMEBUFFER,he);try{let Re=v.textures[pe],Ie=Re.format,Te=Re.type;if(!Pe.textureFormatReadable(Ie)){console.error("THREE.WebGLRenderer.readRenderTargetPixels: renderTarget is not in RGBA or implementation defined format.");return}if(!Pe.textureTypeReadable(Te)){console.error("THREE.WebGLRenderer.readRenderTargetPixels: renderTarget is not in UnsignedByteType or implementation defined type.");return}D>=0&&D<=v.width-B&&F>=0&&F<=v.height-N&&(v.textures.length>1&&A.readBuffer(A.COLOR_ATTACHMENT0+pe),A.readPixels(D,F,B,N,be.convert(Ie),be.convert(Te),J))}finally{let Re=O!==null?_e.get(O).__webglFramebuffer:null;ge.bindFramebuffer(A.FRAMEBUFFER,Re)}}},this.readRenderTargetPixelsAsync=async function(v,D,F,B,N,J,ce,pe=0){if(!(v&&v.isWebGLRenderTarget))throw new Error("THREE.WebGLRenderer.readRenderTargetPixels: renderTarget is not THREE.WebGLRenderTarget.");let he=_e.get(v).__webglFramebuffer;if(v.isWebGLCubeRenderTarget&&ce!==void 0&&(he=he[ce]),he)if(D>=0&&D<=v.width-B&&F>=0&&F<=v.height-N){ge.bindFramebuffer(A.FRAMEBUFFER,he);let Re=v.textures[pe],Ie=Re.format,Te=Re.type;if(!Pe.textureFormatReadable(Ie))throw new Error("THREE.WebGLRenderer.readRenderTargetPixelsAsync: renderTarget is not in RGBA or implementation defined format.");if(!Pe.textureTypeReadable(Te))throw new Error("THREE.WebGLRenderer.readRenderTargetPixelsAsync: renderTarget is not in UnsignedByteType or implementation defined type.");let Ze=A.createBuffer();A.bindBuffer(A.PIXEL_PACK_BUFFER,Ze),A.bufferData(A.PIXEL_PACK_BUFFER,J.byteLength,A.STREAM_READ),v.textures.length>1&&A.readBuffer(A.COLOR_ATTACHMENT0+pe),A.readPixels(D,F,B,N,be.convert(Ie),be.convert(Te),0);let ct=O!==null?_e.get(O).__webglFramebuffer:null;ge.bindFramebuffer(A.FRAMEBUFFER,ct);let wt=A.fenceSync(A.SYNC_GPU_COMMANDS_COMPLETE,0);return A.flush(),await xp(A,wt,4),A.bindBuffer(A.PIXEL_PACK_BUFFER,Ze),A.getBufferSubData(A.PIXEL_PACK_BUFFER,0,J),A.deleteBuffer(Ze),A.deleteSync(wt),J}else throw new Error("THREE.WebGLRenderer.readRenderTargetPixelsAsync: requested read bounds are out of range.")},this.copyFramebufferToTexture=function(v,D=null,F=0){let B=Math.pow(2,-F),N=Math.floor(v.image.width*B),J=Math.floor(v.image.height*B),ce=D!==null?D.x:0,pe=D!==null?D.y:0;$e.setTexture2D(v,0),A.copyTexSubImage2D(A.TEXTURE_2D,F,0,0,ce,pe,N,J),ge.unbindTexture()};let z0=A.createFramebuffer(),V0=A.createFramebuffer();this.copyTextureToTexture=function(v,D,F=null,B=null,N=0,J=null){J===null&&(N!==0?(qr("WebGLRenderer: copyTextureToTexture function signature has changed to support src and dst mipmap levels."),J=N,N=0):J=0);let ce,pe,he,Re,Ie,Te,Ze,ct,wt,mt=v.isCompressedTexture?v.mipmaps[J]:v.image;if(F!==null)ce=F.max.x-F.min.x,pe=F.max.y-F.min.y,he=F.isBox3?F.max.z-F.min.z:1,Re=F.min.x,Ie=F.min.y,Te=F.isBox3?F.min.z:0;else{let Mn=Math.pow(2,-N);ce=Math.floor(mt.width*Mn),pe=Math.floor(mt.height*Mn),v.isDataArrayTexture?he=mt.depth:v.isData3DTexture?he=Math.floor(mt.depth*Mn):he=1,Re=0,Ie=0,Te=0}B!==null?(Ze=B.x,ct=B.y,wt=B.z):(Ze=0,ct=0,wt=0);let ht=be.convert(D.format),Ae=be.convert(D.type),Mt;D.isData3DTexture?($e.setTexture3D(D,0),Mt=A.TEXTURE_3D):D.isDataArrayTexture||D.isCompressedArrayTexture?($e.setTexture2DArray(D,0),Mt=A.TEXTURE_2D_ARRAY):($e.setTexture2D(D,0),Mt=A.TEXTURE_2D),A.pixelStorei(A.UNPACK_FLIP_Y_WEBGL,D.flipY),A.pixelStorei(A.UNPACK_PREMULTIPLY_ALPHA_WEBGL,D.premultiplyAlpha),A.pixelStorei(A.UNPACK_ALIGNMENT,D.unpackAlignment);let it=A.getParameter(A.UNPACK_ROW_LENGTH),mn=A.getParameter(A.UNPACK_IMAGE_HEIGHT),Rr=A.getParameter(A.UNPACK_SKIP_PIXELS),gn=A.getParameter(A.UNPACK_SKIP_ROWS),Es=A.getParameter(A.UNPACK_SKIP_IMAGES);A.pixelStorei(A.UNPACK_ROW_LENGTH,mt.width),A.pixelStorei(A.UNPACK_IMAGE_HEIGHT,mt.height),A.pixelStorei(A.UNPACK_SKIP_PIXELS,Re),A.pixelStorei(A.UNPACK_SKIP_ROWS,Ie),A.pixelStorei(A.UNPACK_SKIP_IMAGES,Te);let Et=v.isDataArrayTexture||v.isData3DTexture,bn=D.isDataArrayTexture||D.isData3DTexture;if(v.isDepthTexture){let Mn=_e.get(v),Zt=_e.get(D),nn=_e.get(Mn.__renderTarget),wl=_e.get(Zt.__renderTarget);ge.bindFramebuffer(A.READ_FRAMEBUFFER,nn.__webglFramebuffer),ge.bindFramebuffer(A.DRAW_FRAMEBUFFER,wl.__webglFramebuffer);for(let Gi=0;Gi<he;Gi++)Et&&(A.framebufferTextureLayer(A.READ_FRAMEBUFFER,A.COLOR_ATTACHMENT0,_e.get(v).__webglTexture,N,Te+Gi),A.framebufferTextureLayer(A.DRAW_FRAMEBUFFER,A.COLOR_ATTACHMENT0,_e.get(D).__webglTexture,J,wt+Gi)),A.blitFramebuffer(Re,Ie,ce,pe,Ze,ct,ce,pe,A.DEPTH_BUFFER_BIT,A.NEAREST);ge.bindFramebuffer(A.READ_FRAMEBUFFER,null),ge.bindFramebuffer(A.DRAW_FRAMEBUFFER,null)}else if(N!==0||v.isRenderTargetTexture||_e.has(v)){let Mn=_e.get(v),Zt=_e.get(D);ge.bindFramebuffer(A.READ_FRAMEBUFFER,z0),ge.bindFramebuffer(A.DRAW_FRAMEBUFFER,V0);for(let nn=0;nn<he;nn++)Et?A.framebufferTextureLayer(A.READ_FRAMEBUFFER,A.COLOR_ATTACHMENT0,Mn.__webglTexture,N,Te+nn):A.framebufferTexture2D(A.READ_FRAMEBUFFER,A.COLOR_ATTACHMENT0,A.TEXTURE_2D,Mn.__webglTexture,N),bn?A.framebufferTextureLayer(A.DRAW_FRAMEBUFFER,A.COLOR_ATTACHMENT0,Zt.__webglTexture,J,wt+nn):A.framebufferTexture2D(A.DRAW_FRAMEBUFFER,A.COLOR_ATTACHMENT0,A.TEXTURE_2D,Zt.__webglTexture,J),N!==0?A.blitFramebuffer(Re,Ie,ce,pe,Ze,ct,ce,pe,A.COLOR_BUFFER_BIT,A.NEAREST):bn?A.copyTexSubImage3D(Mt,J,Ze,ct,wt+nn,Re,Ie,ce,pe):A.copyTexSubImage2D(Mt,J,Ze,ct,Re,Ie,ce,pe);ge.bindFramebuffer(A.READ_FRAMEBUFFER,null),ge.bindFramebuffer(A.DRAW_FRAMEBUFFER,null)}else bn?v.isDataTexture||v.isData3DTexture?A.texSubImage3D(Mt,J,Ze,ct,wt,ce,pe,he,ht,Ae,mt.data):D.isCompressedArrayTexture?A.compressedTexSubImage3D(Mt,J,Ze,ct,wt,ce,pe,he,ht,mt.data):A.texSubImage3D(Mt,J,Ze,ct,wt,ce,pe,he,ht,Ae,mt):v.isDataTexture?A.texSubImage2D(A.TEXTURE_2D,J,Ze,ct,ce,pe,ht,Ae,mt.data):v.isCompressedTexture?A.compressedTexSubImage2D(A.TEXTURE_2D,J,Ze,ct,mt.width,mt.height,ht,mt.data):A.texSubImage2D(A.TEXTURE_2D,J,Ze,ct,ce,pe,ht,Ae,mt);A.pixelStorei(A.UNPACK_ROW_LENGTH,it),A.pixelStorei(A.UNPACK_IMAGE_HEIGHT,mn),A.pixelStorei(A.UNPACK_SKIP_PIXELS,Rr),A.pixelStorei(A.UNPACK_SKIP_ROWS,gn),A.pixelStorei(A.UNPACK_SKIP_IMAGES,Es),J===0&&D.generateMipmaps&&A.generateMipmap(Mt),ge.unbindTexture()},this.initRenderTarget=function(v){_e.get(v).__webglFramebuffer===void 0&&$e.setupRenderTarget(v)},this.initTexture=function(v){v.isCubeTexture?$e.setTextureCube(v,0):v.isData3DTexture?$e.setTexture3D(v,0):v.isDataArrayTexture||v.isCompressedArrayTexture?$e.setTexture2DArray(v,0):$e.setTexture2D(v,0),ge.unbindTexture()},this.resetState=function(){R=0,P=0,O=null,ge.reset(),oe.reset()},typeof __THREE_DEVTOOLS__<"u"&&__THREE_DEVTOOLS__.dispatchEvent(new CustomEvent("observe",{detail:this}))}get coordinateSystem(){return Un}get outputColorSpace(){return this._outputColorSpace}set outputColorSpace(e){this._outputColorSpace=e;let t=this.getContext();t.drawingBufferColorSpace=tt._getDrawingBufferColorSpace(e),t.unpackColorSpace=tt._getUnpackColorSpace()}};var d0=j0(l0(),1);var u0={$schema:"https://json-schema.org/draft/2020-12/schema",$id:"urn:hcp:study-snapshot:1",title:"HCP static study capture v1",type:"object",additionalProperties:!1,required:["schema","version","capture","frame","bindings","particles","source"],properties:{schema:{const:"hcp-study-snapshot"},version:{const:1},numerics:{type:"object",additionalProperties:!1,required:["profile","storage_bits","body_fraction_bits","field_fraction_bits"],properties:{profile:{enum:["legacy_f32","trial_f64"]},storage_bits:{enum:[32,64]},body_fraction_bits:{type:"integer",minimum:0,maximum:52},field_fraction_bits:{type:"integer",minimum:0,maximum:52}},oneOf:[{properties:{profile:{const:"legacy_f32"},storage_bits:{const:32},body_fraction_bits:{const:23},field_fraction_bits:{const:23}}},{properties:{profile:{const:"trial_f64"},storage_bits:{const:64}}}]},response_trace:{type:"object",additionalProperties:!1,required:["status","generation","tick","frame","work_limit","work_required","work_used","readback_bytes","capacity","count","packets","rows"],properties:{status:{enum:["ok","budget_refused","unavailable"]},generation:{anyOf:[{$ref:"#/$defs/decimal"},{type:"null"}]},tick:{$ref:"#/$defs/decimal"},frame:{$ref:"#/$defs/point"},work_limit:{$ref:"#/$defs/decimal"},work_required:{$ref:"#/$defs/decimal"},work_used:{$ref:"#/$defs/decimal"},readback_bytes:{$ref:"#/$defs/decimal"},capacity:{$ref:"#/$defs/decimal"},count:{$ref:"#/$defs/decimal"},packets:{type:"array",items:{type:"object",additionalProperties:!1,required:["slot","comparison"],properties:{slot:{type:"integer",minimum:0},comparison:{$ref:"#/$defs/decimal"}}}},rows:{type:"array",items:{type:"object",additionalProperties:!1,required:["body","child","parent","kind","numerator","denominator","delta","recipient","body_mass","source_mass","production_eligible","extraction_valid"],properties:{body:{$ref:"#/$defs/nativeRecord"},child:{type:"integer",minimum:0},parent:{type:"integer",minimum:0},kind:{enum:["local","inherited"]},numerator:{$ref:"#/$defs/decimal"},denominator:{$ref:"#/$defs/decimal"},delta:{$ref:"#/$defs/extractedPoint"},recipient:{$ref:"#/$defs/extractedPoint"},body_mass:{type:["number","null"]},source_mass:{type:["number","null"]},production_eligible:{type:"boolean"},extraction_valid:{type:"boolean"}}}}}},intent:{type:"object",additionalProperties:!1,required:["status","gravity","generation","tick","frame","work_limit","work_required","work_used","readback_bytes","rows"],properties:{status:{enum:["ok","budget_refused","unavailable"]},gravity:{type:"number",minimum:0},generation:{anyOf:[{$ref:"#/$defs/decimal"},{type:"null"}]},tick:{$ref:"#/$defs/decimal"},frame:{$ref:"#/$defs/point"},work_limit:{$ref:"#/$defs/decimal"},work_required:{$ref:"#/$defs/decimal"},work_used:{$ref:"#/$defs/decimal"},readback_bytes:{$ref:"#/$defs/decimal"},rows:{type:"array",items:{type:"object",additionalProperties:!1,required:["slot","status","relative","position"],properties:{slot:{type:"integer",minimum:0},status:{type:"integer",minimum:0,maximum:3},relative:{anyOf:[{$ref:"#/$defs/point"},{type:"null"}]},position:{anyOf:[{$ref:"#/$defs/point"},{type:"null"}]}}}}}},capture:{type:"object",additionalProperties:!1,required:["session","study","generation","tick","phase","label"],properties:{session:{$ref:"#/$defs/uuid"},study:{$ref:"#/$defs/uuid"},generation:{$ref:"#/$defs/uuid"},tick:{$ref:"#/$defs/decimal"},phase:{enum:["published_seeded","post_tick"]},label:{type:"string"}}},frame:{type:"object",additionalProperties:!1,required:["coordinates","units","reference","camera_target","target_kind","focal_fields"],properties:{coordinates:{const:"reference_local"},units:{const:"operative_model_units"},reference:{$ref:"#/$defs/point"},camera_target:{$ref:"#/$defs/point"},target_kind:{enum:["legacy_origin","captured_focal_centroid"]},focal_fields:{type:"array",items:{type:"object",additionalProperties:!1,required:["kind","value","position","mass"],properties:{kind:{enum:["sibling_definition","parent_value","membership"]},value:{type:"string",minLength:1},position:{$ref:"#/$defs/point"},mass:{type:"number",exclusiveMinimum:0}}}}}},bindings:{type:"array",minItems:1,items:{type:"object",additionalProperties:!1,required:["id","parent","role","units","level"],properties:{id:{type:"integer",minimum:0},parent:{type:"integer",minimum:-1},role:{enum:["root","primary_step","secondary_compression","naming_equivalent"]},units:{type:"number",exclusiveMinimum:0},level:{type:"integer",minimum:0}}}},particles:{type:"array",items:{type:"object",additionalProperties:!1,required:["id","definition","slot","position","radius","mass","exact_mass","binding","level","exposure","aggregate","label"],properties:{id:{type:"string",minLength:1},definition:{type:"string",minLength:1},slot:{type:"integer",minimum:0},position:{$ref:"#/$defs/point"},radius:{type:["number","null"],minimum:0},mass:{type:["number","null"],minimum:0},exact_mass:{$ref:"#/$defs/decimal"},binding:{type:"integer",minimum:0},level:{type:"integer",minimum:0},exposure:{enum:["aggregate","parts","standalone"]},aggregate:{type:"string",minLength:1},label:{type:"string"}}}},execution:{type:"object",additionalProperties:!1,required:["backend","mode","completed_tick","prepared_generation","body_records","comparisons"],properties:{backend:{enum:["native_cpu","native_cuda"]},mode:{enum:["flat","installed_hierarchy"]},completed_tick:{$ref:"#/$defs/decimal"},prepared_generation:{anyOf:[{$ref:"#/$defs/decimal"},{type:"null"}]},body_records:{type:"array",items:{$ref:"#/$defs/nativeRecord"}},comparisons:{type:"array",items:{type:"object",additionalProperties:!1,required:["comparison","field","level","calculation_bits","storage_bits","fraction_bits"],properties:{comparison:{$ref:"#/$defs/decimal"},field:{$ref:"#/$defs/nativeRecord"},level:{type:"integer",minimum:0},calculation_bits:{const:64},storage_bits:{const:64},fraction_bits:{type:"integer",minimum:0,maximum:52}}}}}},source:{type:"object"}},$defs:{extractedPoint:{type:"array",items:{type:["number","null"]},minItems:3,maxItems:3},nativeRecord:{type:"object",additionalProperties:!1,required:["context","incarnation","address","record"],properties:{context:{$ref:"#/$defs/decimal"},incarnation:{$ref:"#/$defs/decimal"},address:{type:"integer",minimum:0},record:{type:"integer",minimum:0}}},uuid:{type:"string",pattern:"^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$",not:{const:"00000000-0000-0000-0000-000000000000"}},decimal:{type:"string",pattern:"^(0|[1-9][0-9]*)$"},point:{type:"array",items:{type:"number"},minItems:3,maxItems:3}}};var h0=new d0.default({allErrors:!0,strict:!0}).compile(u0),fC=(1n<<64n)-1n;function Ye(n,e){if(!n)throw new Error(e)}function Yt(n){Ye(BigInt(n)<=fC,"Exact integer exceeds uint64 range.")}function pC(n){Ye(h0(n),`Unsupported or malformed snapshot: ${h0.errors?.slice(0,3).map(l=>`${l.instancePath||"/"} ${l.message}`).join("; ")}`);let{capture:e,bindings:t,particles:i,frame:r}=n;if(Yt(e.tick),n.execution){let l=n.execution;Ye(l.completed_tick===e.tick,"Native completed tick disagrees with capture."),Yt(l.completed_tick),l.prepared_generation!==null&&Yt(l.prepared_generation);let h=new Set,f=new Set,p=new Set,g=_=>(Yt(_.context),Yt(_.incarnation),Ye(BigInt(_.context)>0n&&BigInt(_.incarnation)>0n,"Missing native record identity."),`${_.context}:${_.incarnation}:${_.address}:${_.record}`);for(let _ of l.body_records)g(_),Ye(!h.has(_.record),"Duplicate native body record."),h.add(_.record);l.prepared_generation!==null&&Ye(h.size===i.length&&i.every(_=>h.has(_.slot)),"Incomplete native occurrence mapping.");for(let _ of l.comparisons){Yt(_.comparison);let m=g(_.field);Ye(!f.has(_.comparison)&&!p.has(m),"Duplicate native comparison identity."),f.add(_.comparison),p.add(m)}Ye(l.mode==="installed_hierarchy"==l.comparisons.length>0,"Hierarchy mode disagrees with comparison metadata."),l.mode==="installed_hierarchy"&&Ye(n.numerics?.profile==="trial_f64"&&l.prepared_generation!==null,"Missing hierarchy precision/generation.")}if(Ye(e.tick==="0"==(e.phase==="published_seeded"),"Tick and capture phase disagree."),n.intent){let l=n.intent,h=n.execution;for(let f of["tick","work_limit","work_required","work_used","readback_bytes"])Yt(l[f]);if(l.generation!==null?Yt(l.generation):Ye(h?.mode==="flat"&&l.status==="unavailable"&&l.work_required==="0","Unprepared execution cannot supply candidate results."),Ye(h&&l.generation===h.prepared_generation&&l.tick===e.tick&&l.frame.every((f,p)=>Number.isFinite(f)&&f===r.reference[p]),"Candidate frame/generation/tick mismatch."),Ye(Number.isFinite(l.gravity),"Invalid candidate gravity."),l.status==="ok"){Ye(h.mode==="installed_hierarchy"&&BigInt(l.work_used)===BigInt(l.work_required)&&BigInt(l.work_used)<=BigInt(l.work_limit),"Invalid candidate work metadata.");let f=new Set,p=new Set(i.map(g=>g.slot));for(let g of l.rows)Ye(!f.has(g.slot)&&p.has(g.slot),"Invalid candidate occurrence mapping."),f.add(g.slot),Ye(g.status===1?Array.isArray(g.position)&&Array.isArray(g.relative)&&[...g.position,...g.relative].every(Number.isFinite):g.position===null&&g.relative===null,"Unsupported candidate coordinates must be unavailable.");Ye(f.size===i.length,"Incomplete candidate mapping.")}else Ye(l.rows.length===0&&l.work_used==="0"&&l.readback_bytes==="0","Failed candidate observation contains results.")}if(n.response_trace){let l=n.response_trace,h=n.execution;for(let f of["tick","work_limit","work_required","work_used","readback_bytes","capacity","count"])Yt(l[f]);if(l.generation!==null&&Yt(l.generation),Ye(h&&l.generation===h.prepared_generation&&l.tick===e.tick&&l.frame.every((f,p)=>Number.isFinite(f)&&f===r.reference[p]),"Response frame/generation/tick mismatch."),l.status==="ok"){Ye(h.mode==="installed_hierarchy"&&l.generation!==null&&BigInt(l.count)===BigInt(l.rows.length)&&BigInt(l.count)<=BigInt(l.capacity)&&BigInt(l.work_required)===1n+BigInt(l.count)+16n*BigInt(l.capacity)&&BigInt(l.work_used)===BigInt(l.work_required)&&BigInt(l.work_used)<=BigInt(l.work_limit)&&BigInt(l.readback_bytes)===112n*BigInt(l.capacity),"Invalid response shape/work metadata.");let f=new Map,p=new Set;for(let g of l.packets)Yt(g.comparison),Ye(!f.has(g.slot)&&!p.has(g.comparison)&&h.comparisons.some(_=>_.comparison===g.comparison),"Invalid response packet mapping."),f.set(g.slot,g.comparison),p.add(g.comparison);Ye(f.size===h.comparisons.length,"Incomplete response packet mapping.");for(let g of l.rows){Yt(g.numerator),Yt(g.denominator),Yt(g.body.context),Yt(g.body.incarnation),Ye(h.body_records.some(m=>Object.keys(m).every(d=>m[d]===g.body[d]))&&f.has(g.child)&&f.has(g.parent)&&(g.kind!=="local"||g.child===g.parent),"Invalid response recipient/path mapping.");let _=[...g.delta,...g.recipient,g.body_mass,g.source_mass];Ye(_.every(m=>m===null||Number.isFinite(m))&&(!g.extraction_valid||_.every(Number.isFinite)),"Invalid response extraction values.")}}else Ye(l.rows.length===0&&l.packets.length===0&&l.work_used==="0"&&l.readback_bytes==="0"&&l.capacity==="0"&&l.count==="0"&&(l.status!=="unavailable"||l.work_required==="0")&&(l.generation!==null||h.mode==="flat"&&l.status==="unavailable"),"Failed response observation contains results.")}t.forEach((l,h)=>{Ye(l.id===h,"Binding IDs must be contiguous and ordered."),h===0?Ye(l.role==="root"&&l.parent===-1&&l.level===0,"Invalid conceptual root."):(Ye(l.role!=="root"&&l.parent>=0&&l.parent<h,"Invalid binding ancestry."),Ye(l.level===t[l.parent].level+(l.role==="primary_step"?1:0),"Conceptual level disagrees with explicit roles."),l.role!=="primary_step"&&Ye(l.units===t[l.parent].units,"Compression/naming must inherit units."))});let s=new Set,a=new Set;for(let l of i)Ye(!s.has(l.id)&&!a.has(l.slot),"Duplicate particle identity or slot."),s.add(l.id),a.add(l.slot),Ye(l.id.startsWith(`${e.study}/`)&&l.aggregate.startsWith(`${e.study}/`),"Mixed study occurrence identities."),Ye(t[l.binding]&&l.level===t[l.binding].level,"Missing or inconsistent conceptual binding."),Ye([...l.position,l.radius,l.mass].every(h=>h===null||Number.isFinite(h)&&(n.numerics?.profile==="trial_f64"||Math.fround(h)===h)),"Particle scalars must preserve the declared native representation."),Yt(l.exact_mass);let o=new Set,c=0,u=[0,0,0];for(let l of r.focal_fields){let h=`${l.kind}:${l.value}`;Ye(!o.has(h),"Duplicate focal field."),o.add(h),c||(u=[...l.position]);let f=c+l.mass;u=u.map((p,g)=>p+(l.position[g]-p)*(l.mass/f)),c=f}return c||(u=r.reference.map(l=>-l)),Ye(r.target_kind===(c?"captured_focal_centroid":"legacy_origin"),"Focal target kind disagrees with selected fields."),Ye(u.every((l,h)=>Number.isFinite(l)&&Math.abs(l-r.camera_target[h])<=1e-12*Math.max(1,Math.abs(l))),"Camera target disagrees with captured focal fields."),n}function $d(n){return pC(JSON.parse(n))}function f0(n,e){let t=n.intent;if(!t)return{"Field intent":"Not requested"};if(t.status!=="ok")return{"Field intent":t.status==="budget_refused"?"Observation budget refused":"Observation unavailable"};let i=t.rows.find(r=>r.slot===e.slot);return{"Field intent":["Absent","Supported","Unresolved coincidence","Unsupported"][i.status],"Intent gravity":t.gravity,"Intent local position":i.position?.join(", ")??"Unavailable","Intent relative offset":i.relative?.join(", ")??"Unavailable"}}function p0(n){let e=new Map;for(let t of n.particles){e.has(t.level)||e.set(t.level,{min:null,max:null,known:0,unknown:0,total:0});let i=e.get(t.level);++i.total,t.mass===null?++i.unknown:(i.min=i.min===null?t.mass:Math.min(i.min,t.mass),i.max=i.max===null?t.mass:Math.max(i.max,t.mass),++i.known)}return e}function pl(n,e){if(n===null)return"#9ba4b4";let t=e.min===e.max?.5:(n-e.min)/(e.max-e.min),i=[[70,100,170],[32,139,163],[108,190,122],[250,225,70]],r=Math.max(0,Math.min(1,t))*(i.length-1),s=Math.min(i.length-2,Math.floor(r)),a=r-s;return"#"+i[s].map((o,c)=>Math.round(o+(i[s+1][c]-o)*a).toString(16).padStart(2,"0")).join("")}function m0(n,{level:e="",exposure:t="",text:i=""}={}){let r=i.toLocaleLowerCase(),s=n.particles.filter(a=>(e===""||a.level===Number(e))&&(!t||a.exposure===t)&&(!r||`${a.id} ${a.definition} ${a.label}`.toLocaleLowerCase().includes(r)));return{matched:s,drawable:s.filter(a=>a.radius!==null&&a.radius>0),unavailable:s.filter(a=>a.radius===null).length,zero:s.filter(a=>a.radius===0).length}}function g0(n){if(!n.length)return{centre:[0,0,0],radius:1};let e=[1/0,1/0,1/0],t=[-1/0,-1/0,-1/0];for(let r of n)for(let s=0;s<3;++s)e[s]=Math.min(e[s],r.position[s]-(r.radius??0)),t[s]=Math.max(t[s],r.position[s]+(r.radius??0));let i=e.map((r,s)=>r+(t[s]-r)/2);return{centre:i,radius:Math.max(...t.map((r,s)=>r-i[s]))}}function _0(n,e){let t=n?.capture,i=e.capture;return!t||t.session!==i.session||t.study!==i.study?"replace":t.generation===i.generation&&t.tick===i.tick&&JSON.stringify(n.intent??null)===JSON.stringify(e.intent??null)?"same":"update"}var mC=100,Hd=1e3;function ml(n,e=Hd){let t=Math.round(Number(n));return n===""||n===null||!Number.isFinite(t)?e:Math.min(36e5,Math.max(mC,t))}var y0=(n,e)=>e.map((t,i)=>t-n[i]),Gd=n=>`${n.lastModified}:${n.size}`,v0=(n,e)=>n.find(t=>t.id===e)??null;function x0({paused:n,error:e,missing:t,hasCapture:i,idleMs:r,intervalMs:s}){return n?"PAUSED":e?"ERROR":!i||t?"WAITING":r>Math.max(1e4,10*s)?"NO CHANGE":"LIVE"}var S0={type:"change"},qd={type:"start"},M0={type:"end"},gl=new er,b0=new wn,gC=Math.cos(70*sa.DEG2RAD),Dt=new L,pn=2*Math.PI,ut={NONE:-1,ROTATE:0,DOLLY:1,PAN:2,TOUCH_ROTATE:3,TOUCH_PAN:4,TOUCH_DOLLY_PAN:5,TOUCH_DOLLY_ROTATE:6},Wd=1e-6,_l=class extends Js{constructor(e,t=null){super(e,t),this.state=ut.NONE,this.target=new L,this.cursor=new L,this.minDistance=0,this.maxDistance=1/0,this.minZoom=0,this.maxZoom=1/0,this.minTargetRadius=0,this.maxTargetRadius=1/0,this.minPolarAngle=0,this.maxPolarAngle=Math.PI,this.minAzimuthAngle=-1/0,this.maxAzimuthAngle=1/0,this.enableDamping=!1,this.dampingFactor=.05,this.enableZoom=!0,this.zoomSpeed=1,this.enableRotate=!0,this.rotateSpeed=1,this.keyRotateSpeed=1,this.enablePan=!0,this.panSpeed=1,this.screenSpacePanning=!0,this.keyPanSpeed=7,this.zoomToCursor=!1,this.autoRotate=!1,this.autoRotateSpeed=2,this.keys={LEFT:"ArrowLeft",UP:"ArrowUp",RIGHT:"ArrowRight",BOTTOM:"ArrowDown"},this.mouseButtons={LEFT:Pi.ROTATE,MIDDLE:Pi.DOLLY,RIGHT:Pi.PAN},this.touches={ONE:Ii.ROTATE,TWO:Ii.DOLLY_PAN},this.target0=this.target.clone(),this.position0=this.object.position.clone(),this.zoom0=this.object.zoom,this._domElementKeyEvents=null,this._lastPosition=new L,this._lastQuaternion=new an,this._lastTargetPosition=new L,this._quat=new an().setFromUnitVectors(e.up,new L(0,1,0)),this._quatInverse=this._quat.clone().invert(),this._spherical=new Kr,this._sphericalDelta=new Kr,this._scale=1,this._panOffset=new L,this._rotateStart=new Ue,this._rotateEnd=new Ue,this._rotateDelta=new Ue,this._panStart=new Ue,this._panEnd=new Ue,this._panDelta=new Ue,this._dollyStart=new Ue,this._dollyEnd=new Ue,this._dollyDelta=new Ue,this._dollyDirection=new L,this._mouse=new Ue,this._performCursorZoom=!1,this._pointers=[],this._pointerPositions={},this._controlActive=!1,this._onPointerMove=yC.bind(this),this._onPointerDown=_C.bind(this),this._onPointerUp=vC.bind(this),this._onContextMenu=TC.bind(this),this._onMouseWheel=bC.bind(this),this._onKeyDown=MC.bind(this),this._onTouchStart=EC.bind(this),this._onTouchMove=wC.bind(this),this._onMouseDown=xC.bind(this),this._onMouseMove=SC.bind(this),this._interceptControlDown=AC.bind(this),this._interceptControlUp=RC.bind(this),this.domElement!==null&&this.connect(this.domElement),this.update()}connect(e){super.connect(e),this.domElement.addEventListener("pointerdown",this._onPointerDown),this.domElement.addEventListener("pointercancel",this._onPointerUp),this.domElement.addEventListener("contextmenu",this._onContextMenu),this.domElement.addEventListener("wheel",this._onMouseWheel,{passive:!1}),this.domElement.getRootNode().addEventListener("keydown",this._interceptControlDown,{passive:!0,capture:!0}),this.domElement.style.touchAction="none"}disconnect(){this.domElement.removeEventListener("pointerdown",this._onPointerDown),this.domElement.removeEventListener("pointermove",this._onPointerMove),this.domElement.removeEventListener("pointerup",this._onPointerUp),this.domElement.removeEventListener("pointercancel",this._onPointerUp),this.domElement.removeEventListener("wheel",this._onMouseWheel),this.domElement.removeEventListener("contextmenu",this._onContextMenu),this.stopListenToKeyEvents(),this.domElement.getRootNode().removeEventListener("keydown",this._interceptControlDown,{capture:!0}),this.domElement.style.touchAction="auto"}dispose(){this.disconnect()}getPolarAngle(){return this._spherical.phi}getAzimuthalAngle(){return this._spherical.theta}getDistance(){return this.object.position.distanceTo(this.target)}listenToKeyEvents(e){e.addEventListener("keydown",this._onKeyDown),this._domElementKeyEvents=e}stopListenToKeyEvents(){this._domElementKeyEvents!==null&&(this._domElementKeyEvents.removeEventListener("keydown",this._onKeyDown),this._domElementKeyEvents=null)}saveState(){this.target0.copy(this.target),this.position0.copy(this.object.position),this.zoom0=this.object.zoom}reset(){this.target.copy(this.target0),this.object.position.copy(this.position0),this.object.zoom=this.zoom0,this.object.updateProjectionMatrix(),this.dispatchEvent(S0),this.update(),this.state=ut.NONE}update(e=null){let t=this.object.position;Dt.copy(t).sub(this.target),Dt.applyQuaternion(this._quat),this._spherical.setFromVector3(Dt),this.autoRotate&&this.state===ut.NONE&&this._rotateLeft(this._getAutoRotationAngle(e)),this.enableDamping?(this._spherical.theta+=this._sphericalDelta.theta*this.dampingFactor,this._spherical.phi+=this._sphericalDelta.phi*this.dampingFactor):(this._spherical.theta+=this._sphericalDelta.theta,this._spherical.phi+=this._sphericalDelta.phi);let i=this.minAzimuthAngle,r=this.maxAzimuthAngle;isFinite(i)&&isFinite(r)&&(i<-Math.PI?i+=pn:i>Math.PI&&(i-=pn),r<-Math.PI?r+=pn:r>Math.PI&&(r-=pn),i<=r?this._spherical.theta=Math.max(i,Math.min(r,this._spherical.theta)):this._spherical.theta=this._spherical.theta>(i+r)/2?Math.max(i,this._spherical.theta):Math.min(r,this._spherical.theta)),this._spherical.phi=Math.max(this.minPolarAngle,Math.min(this.maxPolarAngle,this._spherical.phi)),this._spherical.makeSafe(),this.enableDamping===!0?this.target.addScaledVector(this._panOffset,this.dampingFactor):this.target.add(this._panOffset),this.target.sub(this.cursor),this.target.clampLength(this.minTargetRadius,this.maxTargetRadius),this.target.add(this.cursor);let s=!1;if(this.zoomToCursor&&this._performCursorZoom||this.object.isOrthographicCamera)this._spherical.radius=this._clampDistance(this._spherical.radius);else{let a=this._spherical.radius;this._spherical.radius=this._clampDistance(this._spherical.radius*this._scale),s=a!=this._spherical.radius}if(Dt.setFromSpherical(this._spherical),Dt.applyQuaternion(this._quatInverse),t.copy(this.target).add(Dt),this.object.lookAt(this.target),this.enableDamping===!0?(this._sphericalDelta.theta*=1-this.dampingFactor,this._sphericalDelta.phi*=1-this.dampingFactor,this._panOffset.multiplyScalar(1-this.dampingFactor)):(this._sphericalDelta.set(0,0,0),this._panOffset.set(0,0,0)),this.zoomToCursor&&this._performCursorZoom){let a=null;if(this.object.isPerspectiveCamera){let o=Dt.length();a=this._clampDistance(o*this._scale);let c=o-a;this.object.position.addScaledVector(this._dollyDirection,c),this.object.updateMatrixWorld(),s=!!c}else if(this.object.isOrthographicCamera){let o=new L(this._mouse.x,this._mouse.y,0);o.unproject(this.object);let c=this.object.zoom;this.object.zoom=Math.max(this.minZoom,Math.min(this.maxZoom,this.object.zoom/this._scale)),this.object.updateProjectionMatrix(),s=c!==this.object.zoom;let u=new L(this._mouse.x,this._mouse.y,0);u.unproject(this.object),this.object.position.sub(u).add(o),this.object.updateMatrixWorld(),a=Dt.length()}else console.warn("WARNING: OrbitControls.js encountered an unknown camera type - zoom to cursor disabled."),this.zoomToCursor=!1;a!==null&&(this.screenSpacePanning?this.target.set(0,0,-1).transformDirection(this.object.matrix).multiplyScalar(a).add(this.object.position):(gl.origin.copy(this.object.position),gl.direction.set(0,0,-1).transformDirection(this.object.matrix),Math.abs(this.object.up.dot(gl.direction))<gC?this.object.lookAt(this.target):(b0.setFromNormalAndCoplanarPoint(this.object.up,this.target),gl.intersectPlane(b0,this.target))))}else if(this.object.isOrthographicCamera){let a=this.object.zoom;this.object.zoom=Math.max(this.minZoom,Math.min(this.maxZoom,this.object.zoom/this._scale)),a!==this.object.zoom&&(this.object.updateProjectionMatrix(),s=!0)}return this._scale=1,this._performCursorZoom=!1,s||this._lastPosition.distanceToSquared(this.object.position)>Wd||8*(1-this._lastQuaternion.dot(this.object.quaternion))>Wd||this._lastTargetPosition.distanceToSquared(this.target)>Wd?(this.dispatchEvent(S0),this._lastPosition.copy(this.object.position),this._lastQuaternion.copy(this.object.quaternion),this._lastTargetPosition.copy(this.target),!0):!1}_getAutoRotationAngle(e){return e!==null?pn/60*this.autoRotateSpeed*e:pn/60/60*this.autoRotateSpeed}_getZoomScale(e){let t=Math.abs(e*.01);return Math.pow(.95,this.zoomSpeed*t)}_rotateLeft(e){this._sphericalDelta.theta-=e}_rotateUp(e){this._sphericalDelta.phi-=e}_panLeft(e,t){Dt.setFromMatrixColumn(t,0),Dt.multiplyScalar(-e),this._panOffset.add(Dt)}_panUp(e,t){this.screenSpacePanning===!0?Dt.setFromMatrixColumn(t,1):(Dt.setFromMatrixColumn(t,0),Dt.crossVectors(this.object.up,Dt)),Dt.multiplyScalar(e),this._panOffset.add(Dt)}_pan(e,t){let i=this.domElement;if(this.object.isPerspectiveCamera){let r=this.object.position;Dt.copy(r).sub(this.target);let s=Dt.length();s*=Math.tan(this.object.fov/2*Math.PI/180),this._panLeft(2*e*s/i.clientHeight,this.object.matrix),this._panUp(2*t*s/i.clientHeight,this.object.matrix)}else this.object.isOrthographicCamera?(this._panLeft(e*(this.object.right-this.object.left)/this.object.zoom/i.clientWidth,this.object.matrix),this._panUp(t*(this.object.top-this.object.bottom)/this.object.zoom/i.clientHeight,this.object.matrix)):(console.warn("WARNING: OrbitControls.js encountered an unknown camera type - pan disabled."),this.enablePan=!1)}_dollyOut(e){this.object.isPerspectiveCamera||this.object.isOrthographicCamera?this._scale/=e:(console.warn("WARNING: OrbitControls.js encountered an unknown camera type - dolly/zoom disabled."),this.enableZoom=!1)}_dollyIn(e){this.object.isPerspectiveCamera||this.object.isOrthographicCamera?this._scale*=e:(console.warn("WARNING: OrbitControls.js encountered an unknown camera type - dolly/zoom disabled."),this.enableZoom=!1)}_updateZoomParameters(e,t){if(!this.zoomToCursor)return;this._performCursorZoom=!0;let i=this.domElement.getBoundingClientRect(),r=e-i.left,s=t-i.top,a=i.width,o=i.height;this._mouse.x=r/a*2-1,this._mouse.y=-(s/o)*2+1,this._dollyDirection.set(this._mouse.x,this._mouse.y,1).unproject(this.object).sub(this.object.position).normalize()}_clampDistance(e){return Math.max(this.minDistance,Math.min(this.maxDistance,e))}_handleMouseDownRotate(e){this._rotateStart.set(e.clientX,e.clientY)}_handleMouseDownDolly(e){this._updateZoomParameters(e.clientX,e.clientX),this._dollyStart.set(e.clientX,e.clientY)}_handleMouseDownPan(e){this._panStart.set(e.clientX,e.clientY)}_handleMouseMoveRotate(e){this._rotateEnd.set(e.clientX,e.clientY),this._rotateDelta.subVectors(this._rotateEnd,this._rotateStart).multiplyScalar(this.rotateSpeed);let t=this.domElement;this._rotateLeft(pn*this._rotateDelta.x/t.clientHeight),this._rotateUp(pn*this._rotateDelta.y/t.clientHeight),this._rotateStart.copy(this._rotateEnd),this.update()}_handleMouseMoveDolly(e){this._dollyEnd.set(e.clientX,e.clientY),this._dollyDelta.subVectors(this._dollyEnd,this._dollyStart),this._dollyDelta.y>0?this._dollyOut(this._getZoomScale(this._dollyDelta.y)):this._dollyDelta.y<0&&this._dollyIn(this._getZoomScale(this._dollyDelta.y)),this._dollyStart.copy(this._dollyEnd),this.update()}_handleMouseMovePan(e){this._panEnd.set(e.clientX,e.clientY),this._panDelta.subVectors(this._panEnd,this._panStart).multiplyScalar(this.panSpeed),this._pan(this._panDelta.x,this._panDelta.y),this._panStart.copy(this._panEnd),this.update()}_handleMouseWheel(e){this._updateZoomParameters(e.clientX,e.clientY),e.deltaY<0?this._dollyIn(this._getZoomScale(e.deltaY)):e.deltaY>0&&this._dollyOut(this._getZoomScale(e.deltaY)),this.update()}_handleKeyDown(e){let t=!1;switch(e.code){case this.keys.UP:e.ctrlKey||e.metaKey||e.shiftKey?this.enableRotate&&this._rotateUp(pn*this.keyRotateSpeed/this.domElement.clientHeight):this.enablePan&&this._pan(0,this.keyPanSpeed),t=!0;break;case this.keys.BOTTOM:e.ctrlKey||e.metaKey||e.shiftKey?this.enableRotate&&this._rotateUp(-pn*this.keyRotateSpeed/this.domElement.clientHeight):this.enablePan&&this._pan(0,-this.keyPanSpeed),t=!0;break;case this.keys.LEFT:e.ctrlKey||e.metaKey||e.shiftKey?this.enableRotate&&this._rotateLeft(pn*this.keyRotateSpeed/this.domElement.clientHeight):this.enablePan&&this._pan(this.keyPanSpeed,0),t=!0;break;case this.keys.RIGHT:e.ctrlKey||e.metaKey||e.shiftKey?this.enableRotate&&this._rotateLeft(-pn*this.keyRotateSpeed/this.domElement.clientHeight):this.enablePan&&this._pan(-this.keyPanSpeed,0),t=!0;break}t&&(e.preventDefault(),this.update())}_handleTouchStartRotate(e){if(this._pointers.length===1)this._rotateStart.set(e.pageX,e.pageY);else{let t=this._getSecondPointerPosition(e),i=.5*(e.pageX+t.x),r=.5*(e.pageY+t.y);this._rotateStart.set(i,r)}}_handleTouchStartPan(e){if(this._pointers.length===1)this._panStart.set(e.pageX,e.pageY);else{let t=this._getSecondPointerPosition(e),i=.5*(e.pageX+t.x),r=.5*(e.pageY+t.y);this._panStart.set(i,r)}}_handleTouchStartDolly(e){let t=this._getSecondPointerPosition(e),i=e.pageX-t.x,r=e.pageY-t.y,s=Math.sqrt(i*i+r*r);this._dollyStart.set(0,s)}_handleTouchStartDollyPan(e){this.enableZoom&&this._handleTouchStartDolly(e),this.enablePan&&this._handleTouchStartPan(e)}_handleTouchStartDollyRotate(e){this.enableZoom&&this._handleTouchStartDolly(e),this.enableRotate&&this._handleTouchStartRotate(e)}_handleTouchMoveRotate(e){if(this._pointers.length==1)this._rotateEnd.set(e.pageX,e.pageY);else{let i=this._getSecondPointerPosition(e),r=.5*(e.pageX+i.x),s=.5*(e.pageY+i.y);this._rotateEnd.set(r,s)}this._rotateDelta.subVectors(this._rotateEnd,this._rotateStart).multiplyScalar(this.rotateSpeed);let t=this.domElement;this._rotateLeft(pn*this._rotateDelta.x/t.clientHeight),this._rotateUp(pn*this._rotateDelta.y/t.clientHeight),this._rotateStart.copy(this._rotateEnd)}_handleTouchMovePan(e){if(this._pointers.length===1)this._panEnd.set(e.pageX,e.pageY);else{let t=this._getSecondPointerPosition(e),i=.5*(e.pageX+t.x),r=.5*(e.pageY+t.y);this._panEnd.set(i,r)}this._panDelta.subVectors(this._panEnd,this._panStart).multiplyScalar(this.panSpeed),this._pan(this._panDelta.x,this._panDelta.y),this._panStart.copy(this._panEnd)}_handleTouchMoveDolly(e){let t=this._getSecondPointerPosition(e),i=e.pageX-t.x,r=e.pageY-t.y,s=Math.sqrt(i*i+r*r);this._dollyEnd.set(0,s),this._dollyDelta.set(0,Math.pow(this._dollyEnd.y/this._dollyStart.y,this.zoomSpeed)),this._dollyOut(this._dollyDelta.y),this._dollyStart.copy(this._dollyEnd);let a=(e.pageX+t.x)*.5,o=(e.pageY+t.y)*.5;this._updateZoomParameters(a,o)}_handleTouchMoveDollyPan(e){this.enableZoom&&this._handleTouchMoveDolly(e),this.enablePan&&this._handleTouchMovePan(e)}_handleTouchMoveDollyRotate(e){this.enableZoom&&this._handleTouchMoveDolly(e),this.enableRotate&&this._handleTouchMoveRotate(e)}_addPointer(e){this._pointers.push(e.pointerId)}_removePointer(e){delete this._pointerPositions[e.pointerId];for(let t=0;t<this._pointers.length;t++)if(this._pointers[t]==e.pointerId){this._pointers.splice(t,1);return}}_isTrackingPointer(e){for(let t=0;t<this._pointers.length;t++)if(this._pointers[t]==e.pointerId)return!0;return!1}_trackPointer(e){let t=this._pointerPositions[e.pointerId];t===void 0&&(t=new Ue,this._pointerPositions[e.pointerId]=t),t.set(e.pageX,e.pageY)}_getSecondPointerPosition(e){let t=e.pointerId===this._pointers[0]?this._pointers[1]:this._pointers[0];return this._pointerPositions[t]}_customWheelEvent(e){let t=e.deltaMode,i={clientX:e.clientX,clientY:e.clientY,deltaY:e.deltaY};switch(t){case 1:i.deltaY*=16;break;case 2:i.deltaY*=100;break}return e.ctrlKey&&!this._controlActive&&(i.deltaY*=10),i}};function _C(n){this.enabled!==!1&&(this._pointers.length===0&&(this.domElement.setPointerCapture(n.pointerId),this.domElement.addEventListener("pointermove",this._onPointerMove),this.domElement.addEventListener("pointerup",this._onPointerUp)),!this._isTrackingPointer(n)&&(this._addPointer(n),n.pointerType==="touch"?this._onTouchStart(n):this._onMouseDown(n)))}function yC(n){this.enabled!==!1&&(n.pointerType==="touch"?this._onTouchMove(n):this._onMouseMove(n))}function vC(n){switch(this._removePointer(n),this._pointers.length){case 0:this.domElement.releasePointerCapture(n.pointerId),this.domElement.removeEventListener("pointermove",this._onPointerMove),this.domElement.removeEventListener("pointerup",this._onPointerUp),this.dispatchEvent(M0),this.state=ut.NONE;break;case 1:let e=this._pointers[0],t=this._pointerPositions[e];this._onTouchStart({pointerId:e,pageX:t.x,pageY:t.y});break}}function xC(n){let e;switch(n.button){case 0:e=this.mouseButtons.LEFT;break;case 1:e=this.mouseButtons.MIDDLE;break;case 2:e=this.mouseButtons.RIGHT;break;default:e=-1}switch(e){case Pi.DOLLY:if(this.enableZoom===!1)return;this._handleMouseDownDolly(n),this.state=ut.DOLLY;break;case Pi.ROTATE:if(n.ctrlKey||n.metaKey||n.shiftKey){if(this.enablePan===!1)return;this._handleMouseDownPan(n),this.state=ut.PAN}else{if(this.enableRotate===!1)return;this._handleMouseDownRotate(n),this.state=ut.ROTATE}break;case Pi.PAN:if(n.ctrlKey||n.metaKey||n.shiftKey){if(this.enableRotate===!1)return;this._handleMouseDownRotate(n),this.state=ut.ROTATE}else{if(this.enablePan===!1)return;this._handleMouseDownPan(n),this.state=ut.PAN}break;default:this.state=ut.NONE}this.state!==ut.NONE&&this.dispatchEvent(qd)}function SC(n){switch(this.state){case ut.ROTATE:if(this.enableRotate===!1)return;this._handleMouseMoveRotate(n);break;case ut.DOLLY:if(this.enableZoom===!1)return;this._handleMouseMoveDolly(n);break;case ut.PAN:if(this.enablePan===!1)return;this._handleMouseMovePan(n);break}}function bC(n){this.enabled===!1||this.enableZoom===!1||this.state!==ut.NONE||(n.preventDefault(),this.dispatchEvent(qd),this._handleMouseWheel(this._customWheelEvent(n)),this.dispatchEvent(M0))}function MC(n){this.enabled!==!1&&this._handleKeyDown(n)}function EC(n){switch(this._trackPointer(n),this._pointers.length){case 1:switch(this.touches.ONE){case Ii.ROTATE:if(this.enableRotate===!1)return;this._handleTouchStartRotate(n),this.state=ut.TOUCH_ROTATE;break;case Ii.PAN:if(this.enablePan===!1)return;this._handleTouchStartPan(n),this.state=ut.TOUCH_PAN;break;default:this.state=ut.NONE}break;case 2:switch(this.touches.TWO){case Ii.DOLLY_PAN:if(this.enableZoom===!1&&this.enablePan===!1)return;this._handleTouchStartDollyPan(n),this.state=ut.TOUCH_DOLLY_PAN;break;case Ii.DOLLY_ROTATE:if(this.enableZoom===!1&&this.enableRotate===!1)return;this._handleTouchStartDollyRotate(n),this.state=ut.TOUCH_DOLLY_ROTATE;break;default:this.state=ut.NONE}break;default:this.state=ut.NONE}this.state!==ut.NONE&&this.dispatchEvent(qd)}function wC(n){switch(this._trackPointer(n),this.state){case ut.TOUCH_ROTATE:if(this.enableRotate===!1)return;this._handleTouchMoveRotate(n),this.update();break;case ut.TOUCH_PAN:if(this.enablePan===!1)return;this._handleTouchMovePan(n),this.update();break;case ut.TOUCH_DOLLY_PAN:if(this.enableZoom===!1&&this.enablePan===!1)return;this._handleTouchMoveDollyPan(n),this.update();break;case ut.TOUCH_DOLLY_ROTATE:if(this.enableZoom===!1&&this.enableRotate===!1)return;this._handleTouchMoveDollyRotate(n),this.update();break;default:this.state=ut.NONE}}function TC(n){this.enabled!==!1&&n.preventDefault()}function AC(n){n.key==="Control"&&(this._controlActive=!0,this.domElement.getRootNode().addEventListener("keyup",this._interceptControlUp,{passive:!0,capture:!0}))}function RC(n){n.key==="Control"&&(this._controlActive=!1,this.domElement.getRootNode().removeEventListener("keyup",this._interceptControlUp,{passive:!0,capture:!0}))}var xe=n=>document.getElementById(n),vl=xe("space"),xl=new $s,tn=new $t(45,1,.001,1e4);tn.position.set(5,4,7);var Ut,Er,Ve=null,wr,Sn,$i=[],Tr=null,Mr=[],Sl=0,Zd=0,Kd=0,Xd=!1,jd=0,T0=new Ys(1,32,24),CC=new Ai({color:16777215}),bs=new Ht(T0,new Ai({color:16777215,wireframe:!0,transparent:!0,opacity:.35}));bs.visible=!1;xl.add(bs);function gi(n=""){xe("error").textContent=n,xe("error").hidden=!n}function ka(n,e,t){let i=document.createElement("dt"),r=document.createElement("dd");i.textContent=e,r.textContent=t,n.append(i,r)}function A0(){xe("metrics").replaceChildren();for(let[n,e]of[["Parse + validate",Sl],["Build display",Zd],["Last render submission",Kd]])ka(xe("metrics"),n,`${e.toFixed(2)} ms`)}function za(){!Ut||Xd||(Xd=!0,requestAnimationFrame(()=>{Xd=!1;let n=performance.now();Ut.render(xl,tn),Kd=performance.now()-n,A0()}))}function E0(){if(!Ut)return;let n=vl.clientWidth,e=vl.clientHeight;Ut.setSize(n,e),tn.aspect=n/Math.max(1,e),tn.updateProjectionMatrix(),za()}var yl=[0,0,0],Ms=[0,0,0],R0=()=>Ms.some(n=>n!==0),bl=n=>Ms.every(e=>e===0)?n:n.map((e,t)=>e+Ms[t]);function Ml(n,e=null){n=n.map(o=>({position:bl(o.position),radius:o.radius}));let t=g0(n),i=e??(n.length===1?n[0].position:t.centre),r=.001;for(let o of n)r=Math.max(r,Math.hypot(...o.position.map((c,u)=>c-i[u]))+(o.radius??0));let s=r*1.8/Math.sin(sa.degToRad(tn.fov/2))/Math.min(1,tn.aspect),a=tn.position.clone().sub(Er.target).normalize();a.lengthSq()||a.set(1,.7,1).normalize(),Er.target.fromArray(i),tn.position.copy(Er.target).addScaledVector(a,s),tn.near=Math.max(s/1e5,1e-7),tn.far=Math.max(s*1e3,r*1e3),tn.updateProjectionMatrix(),Er.update(),za()}function Yd(n){if(Tr=n,xe("inspection").hidden=!n,xe("frame-selection").disabled=!n,xe("details").replaceChildren(),n){let e=Ve.bindings[n.binding];for(let[t,i]of Object.entries({Label:n.label||"Not supplied",Occurrence:n.id,Definition:n.definition,"Local position":n.position.join(", "),Radius:n.radius??"Unavailable","Numerical mass":n.mass??"Unavailable","Exact structural mass":n.exact_mass,"Conceptual LoD":n.level,Binding:`${n.binding} \xB7 ${e.role} \xB7 ${e.units} units`,Exposure:n.exposure,"Aggregate context":n.aggregate}))ka(xe("details"),t,i);for(let[t,i]of Object.entries(f0(Ve,n)))ka(xe("details"),t,i);bs.visible=n.radius!==null&&n.radius>0&&$i.includes(n),bs.position.fromArray(bl(n.position)),bs.scale.setScalar(n.radius??0)}else bs.visible=!1;for(let e of xe("objects").children)e.setAttribute("aria-pressed",String(e.dataset.id===n?.id));za()}function PC(){xe("legends").replaceChildren();for(let[n,e]of[...wr].sort(([t],[i])=>t-i)){let t=document.createElement("div");t.className="legend";let i=document.createElement("strong");i.textContent=`LoD ${n} \xB7 ${e.total} captured`;let r=document.createElement("div");r.className="spectrum",r.style.background=e.known?e.min===e.max?pl(e.min,e):`linear-gradient(to right, ${[0,1/3,2/3,1].map(o=>pl(e.min+o*(e.max-e.min),e)).join(",")})`:"#9ba4b4";let s=document.createElement("div");s.className="range";for(let o of[e.min,e.max]){let c=document.createElement("span");c.textContent=o??"Unavailable",s.append(c)}t.append(i,r,s);let a=document.createElement("p");a.className="muted",a.textContent=`${e.min===e.max&&e.known?"Equal mass: midpoint colour. ":""}${e.unknown} unavailable (grey).`,t.append(a),xe("legends").append(t)}}function Jd(){if(!Ve||!Ut)return;let n=performance.now(),e=m0(Ve,{level:xe("level").value,exposure:xe("exposure").value,text:xe("search").value});if(Mr=e.matched,$i=e.drawable,Sn&&(xl.remove(Sn),Sn.dispose(),Sn=null),$i.length){Sn=new Gs(T0,CC,$i.length);let t=new cn,i=new Qe;$i.forEach((r,s)=>{t.position.fromArray(bl(r.position)),t.scale.setScalar(r.radius),t.updateMatrix(),Sn.setMatrixAt(s,t.matrix),Sn.setColorAt(s,i.set(pl(r.mass,wr.get(r.level))))}),Sn.computeBoundingSphere(),xl.add(Sn)}xe("counts").textContent=`${Ve.particles.length} total \xB7 ${Mr.length} matched \xB7 ${$i.length} drawable`,xe("omissions").textContent=`${Ve.particles.length-Mr.length} filtered out \xB7 ${e.unavailable} missing extent \xB7 ${e.zero} zero radius. Camera culling is separate; no display cap.`,xe("objects").replaceChildren();for(let t of Mr){let i=document.createElement("button");i.dataset.id=t.id,i.setAttribute("role","listitem"),i.textContent=`${t.label||"Unlabelled"} \xB7 LoD ${t.level}`;let r=document.createElement("small");r.textContent=`${t.id} \xB7 ${t.radius===null?"extent unavailable":t.radius===0?"zero radius":t.exposure}`,i.append(r),i.addEventListener("click",()=>Yd(t)),xe("objects").append(i)}Yd(Tr?v0(Mr,Tr.id):null),Zd=performance.now()-n,A0(),za()}function C0(n){return n&&typeof n=="object"&&(Object.values(n).forEach(C0),Object.freeze(n)),n}function P0(n,e=!1){let t=xe("level").value,i=e?yl:n.frame.reference;Ve=C0(n),wr=p0(Ve),e||(Tr=null),yl=[...i],Ms=y0(yl,Ve.frame.reference),xe("capture-title").textContent=Ve.capture.label||"Untitled study",xe("capture-meta").replaceChildren();for(let[r,s]of Object.entries({Schema:"HCP snapshot v1",Study:Ve.capture.study,Generation:Ve.capture.generation,Tick:`${Ve.capture.tick} \xB7 ${Ve.capture.phase}`,...Ve.execution&&{Backend:Ve.execution.backend,Execution:Ve.execution.mode},...Ve.numerics&&{Numerics:`${Ve.numerics.profile} \xB7 body Q${Ve.numerics.body_fraction_bits} \xB7 default field Q${Ve.numerics.field_fraction_bits}`},...Ve.execution?.comparisons.length&&{"Comparison precision":`${Ve.execution.comparisons.length} comparisons \xB7 retained fraction bits ${[...new Set(Ve.execution.comparisons.map(a=>a.fraction_bits))].sort((a,o)=>a-o).join(", ")}`},"Double frame reference":Ve.frame.reference.join(", "),"Captured camera target":Ve.frame.camera_target.join(", "),...R0()&&{"Display offset":`${Ms.join(", ")} (objects shifted; camera unchanged)`},Coordinates:"Reference-local \xB7 operative model units"}))ka(xe("capture-meta"),r,s);xe("level").replaceChildren(new Option("All captured levels",""));for(let r of[...wr.keys()].sort((s,a)=>s-a))xe("level").add(new Option(`LoD ${r}`,String(r)));e?t!==""&&wr.has(Number(t))&&(xe("level").value=t):(xe("exposure").value="",xe("search").value=""),xe("empty").hidden=!0,xe("frame-all").disabled=!1,xe("focus").disabled=!1,PC(),Jd(),e||Ml(Ve.particles),Ar()}var w0="hcp-viewport-poll-ms",I0=typeof window.showDirectoryPicker=="function",at=null,D0=0,N0={get:()=>{try{return localStorage.getItem(w0)}catch{return null}},set:n=>{try{localStorage.setItem(w0,String(n))}catch{}}},IC=n=>n<6e4?`${Math.floor(n/1e3)} s`:`${Math.floor(n/6e4)} min ${Math.floor(n%6e4/1e3)} s`;function Ar(){let n=at,e=Date.now(),t=n?x0({paused:n.paused,error:n.error,missing:n.missing,hasCapture:!!n.last,idleMs:e-n.changedAt,intervalMs:n.interval}):"STATIC";if(xe("mode").textContent=t,xe("mode").dataset.mode=t,xe("pause").disabled=xe("stop").disabled=!n,xe("pause").textContent=n?.paused?"Resume":"Pause",xe("live-status").replaceChildren(),n)for(let[r,s]of Object.entries({Folder:n.name,State:t,"Latest file":n.missing?"latest.json not found; waiting":n.checks?"latest.json":"Not yet checked","Applied tick":n.last?`${n.last.capture.tick} \xB7 ${n.last.capture.phase}`:"None yet","Last change":n.changedAt?new Date(n.changedAt).toLocaleTimeString():"None yet","Since last change":n.changedAt?IC(e-n.changedAt):"None yet","Last check":n.checkedAt?new Date(n.checkedAt).toLocaleTimeString():"None yet","Checks \xB7 captures applied":`${n.checks} \xB7 ${n.applied}`}))ka(xe("live-status"),r,s);if(!Ve)return;let i=`tick ${Ve.capture.tick} \xB7 actual model radii \xB7 ${Ve.particles.length} objects${R0()?" \xB7 display offset":""}`;xe("frame-note").textContent=n?n.last?`${t} CAPTURE \xB7 ${i} \xB7 polling ${n.name}/latest.json every ${n.interval} ms \xB7 colour ranges per capture`:`STATIC CAPTURE (opened file) \xB7 ${i} \xB7 watching ${n.name}, ${t.toLowerCase()}`:`STATIC CAPTURE \xB7 ${i}`}function L0(n){clearTimeout(n.timer),n.timer=setTimeout(()=>Qd(n),n.interval)}async function Qd(n){if(at!==n||n.paused||n.busy)return;n.busy=!0,++n.checks,n.checkedAt=Date.now();let e=!1;try{let t;try{t=await(await n.src.getFileHandle("latest.json")).getFile(),n.missing=!1}catch(c){if(c?.name!=="NotFoundError")throw c;n.missing=!0,n.error=null,n.stamp=null,n.racing=0,at===n&&gi();return}if(Gd(t)===n.stamp){n.racing=0;return}let i=await t.text();if(n.racing=0,at!==n||n.paused)return;n.stamp=Gd(t),e=!0;let r=performance.now(),s=$d(i),a=performance.now()-r;if(!Ut)throw new Error("WebGL renderer is unavailable.");let o=_0(Ve,s);o!=="same"&&(Sl=a,P0(s,o==="update")),(o!=="same"||!n.last)&&(n.last=Ve,n.changedAt=Date.now(),++n.applied),n.error=null,gi()}catch(t){if(e||(n.stamp=null),t?.name==="NotReadableError"&&++n.racing<=3)return;at===n&&(n.error=t.message,gi(`Could not read latest.json: ${t.message}${n.last?` Capture at tick ${n.last.capture.tick} remains displayed.`:Ve?" Previous capture remains displayed.":""} Still watching.`))}finally{at===n&&(n.busy=!1,n.paused||L0(n),Ar())}}function ef(){at&&(clearTimeout(at.timer),clearInterval(D0),at=null,gi(),Ar())}function U0(n,e){ef(),D0=setInterval(Ar,1e3);let t=at={src:n,name:e,interval:ml(xe("interval").value),paused:!1,busy:!1,racing:0,timer:0,stamp:null,last:null,checks:0,applied:0,changedAt:0,checkedAt:0,missing:!1,error:null};gi(),Ar(),Qd(t)}xe("interval").value=ml(N0.get());xe("interval").addEventListener("change",()=>{let n=ml(xe("interval").value,at?.interval??Hd);xe("interval").value=n,N0.set(n),at&&(at.interval=n,!at.paused&&!at.busy&&L0(at),Ar())});xe("pause").addEventListener("click",()=>{at&&(at.paused=!at.paused,at.racing=0,clearTimeout(at.timer),at.paused?Ar():Qd(at))});xe("stop").addEventListener("click",ef);I0?xe("watch").addEventListener("click",async()=>{try{let n=await window.showDirectoryPicker({mode:"read"});U0(n,n.name)}catch(n){n?.name!=="AbortError"&&gi(`Could not watch folder: ${n.message}`)}}):(xe("watch").disabled=!0,xe("live-note").textContent="Watching a folder needs the File System Access API (Chromium-based browsers on a secure page, including a local file). This browser does not provide it. Open snapshot still works.",xe("watch").title="Not supported in this browser; Open snapshot still works.");xe("file").addEventListener("change",async n=>{let e=++jd,t=n.target.files?.[0];if(t){try{let i=await t.text(),r=performance.now(),s=$d(i),a=performance.now()-r;if(e!==jd)return;if(!Ut)throw new Error("WebGL renderer is unavailable.");Sl=a,ef(),P0(s),gi()}catch(i){e===jd&&gi(`Could not load ${t.name}: ${i.message}${Ve?" Previous capture remains displayed.":""}`)}n.target.value=""}});for(let n of["level","exposure"])xe(n).addEventListener("change",Jd);xe("search").addEventListener("input",Jd);xe("frame-all").addEventListener("click",()=>Ve&&Ml(Mr));xe("frame-selection").addEventListener("click",()=>Tr&&Ml([Tr]));xe("focus").addEventListener("click",()=>Ve&&Ml(Ve.particles,bl(Ve.frame.camera_target)));try{Ut=new Cc({antialias:!0,alpha:!0}),Ut.setPixelRatio(Math.min(devicePixelRatio,2)),Ut.outputColorSpace=Vt,vl.prepend(Ut.domElement),Er=new _l(tn,Ut.domElement),Er.addEventListener("change",za),new ResizeObserver(E0).observe(vl),E0();let n=new Ks,e=new Ue,t;Ut.domElement.addEventListener("pointerdown",i=>{t=[i.clientX,i.clientY]}),Ut.domElement.addEventListener("pointerup",i=>{if(!Ve||!Sn||!t||Math.hypot(i.clientX-t[0],i.clientY-t[1])>4||i.button!==0)return;let r=Ut.domElement.getBoundingClientRect();e.set((i.clientX-r.left)/r.width*2-1,-(i.clientY-r.top)/r.height*2+1),n.setFromCamera(e,tn);let s=n.intersectObject(Sn)[0];Yd(s?$i[s.instanceId]:null)})}catch(n){gi(`WebGL could not start: ${n.message}`)}window.HCPViewport={inspect:()=>({snapshot:Ve?structuredClone(Ve):null,matched:Mr.map(n=>n.id),drawn:$i.map((n,e)=>{let t=new _t;Sn.getMatrixAt(e,t);let i=new L,r=new L,s=new an;t.decompose(i,s,r);let a=new Qe;Sn.getColorAt(e,a);let o=i.clone().project(tn),c=Ut.domElement.getBoundingClientRect();return{id:n.id,position:i.toArray(),radius:r.x,colour:"#"+a.getHexString(Vt),screen:{x:c.left+(o.x+1)*c.width/2,y:c.top+(1-o.y)*c.height/2,z:o.z}}}),selected:Tr?.id??null,ranges:wr?[...wr]:[],camera:{position:tn.position.toArray(),target:Er?.target.toArray()},display:{reference:[...yl],shift:[...Ms]},metrics:{parse_ms:Sl,build_ms:Zd,render_submit_ms:Kd},watch:{supported:I0,secure:isSecureContext,mode:xe("mode").textContent,active:!!at,paused:at?.paused??null,interval:at?.interval??null,checks:at?.checks??0,applied:at?.applied??0,error:at?.error??null,missing:at?.missing??null},renderer:Ut?.getContext().getParameter(Ut.getContext().VERSION)??null}),watchWith:(n,e="test source")=>U0(n,e)};})();
/*! For license information please see viewer.bundle.js.LEGAL.txt */
