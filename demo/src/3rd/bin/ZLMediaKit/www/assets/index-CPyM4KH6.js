import{S as O,T as U}from"./SearchForm-BpKoI_WR.js";import{W}from"./WForm-yLSO6vqa.js";import{_ as A,b as s,u as F,o as _,T as $,w as d,f as G,c as r,h as x,g as p,a0 as R,j as S,k as V,B as L,d as J,V as K,e as h,Z as z,U as T}from"./index-BlIwGkDV.js";import{_ as j}from"./index-BCZz1Xh_.js";import{u as P,_ as Z}from"./index.es-C5-spk3Y.js";import"./index-BktMoYmR.js";import"./Col-B0FqQPE6.js";import"./debounce-CPjBQWpM.js";import"./index-DYyLVBxh.js";import"./List-wnk-fyaR.js";import"./index-BsB2-bWR.js";import"./index-h_ISDNrj.js";import"./index-SO-EjZpI.js";const Q={class:"footer"},X={__name:"Edit",emits:["success"],setup(M,{expose:w,emit:m}){const n=s({}),o=s(!1),f=s(""),C=m,u=s(!1),{SEND_MESSAGE:N}=F(),g=async()=>{if(!await f.value.validate())return;const t={type:"setAiBoxNetwork",...n.value};S.on(t.type,l=>{l.result===0?(V.success(l.msg),o.value=!1,C("success")):V.error(l.msg),u.value=!1,S.off(t.type)}),u.value=!0,N(t)},I=y=>{n.value=JSON.parse(JSON.stringify(y)),o.value=!0,u.value=!1},k=()=>{o.value=!1},D=[{type:"input",label:"网络名称",key:"name"},{type:"select",label:"分配方式",key:"dhcp",list:[{value:0,label:"手动配置"},{value:1,label:"自动配置"}]},{type:"input",label:"网络地址",key:"address"},{type:"input",label:"子网掩码",key:"mask"},{type:"input",label:"网关地址",key:"gateway"},{type:"input",label:"DNS",key:"dns"}],v={name:[{required:!0,message:"请输入",trigger:"blur"}],address:[{required:!0,message:"请输入",trigger:"blur"}],dhcp:[{required:!0,message:"请输入",trigger:"blur"}],mask:[{required:!0,message:"请输入",trigger:"blur"}],gateway:[{required:!0,message:"请输入",trigger:"blur"}],dns:[{required:!0,message:"请输入",trigger:"blur"}]};return w({showDrawer:I}),(y,t)=>{const l=L,E=j;return _(),$(E,{open:p(o),"onUpdate:open":t[1]||(t[1]=c=>R(o)?o.value=c:null),title:"编辑",width:"520","footer-style":{textAlign:"right"},onClose:k},{footer:d(()=>[G("div",Q,[r(l,{type:"primary",onClick:g},{default:d(()=>t[2]||(t[2]=[x("确认")])),_:1}),r(l,{class:"close",onClick:k},{default:d(()=>t[3]||(t[3]=[x("取消")])),_:1})])]),default:d(()=>[r(W,{ref_key:"wFormRef",ref:f,modelValue:p(n),"onUpdate:modelValue":t[0]||(t[0]=c=>R(n)?n.value=c:null),formList:D,rules:v,labelWidth:"100px"},null,8,["modelValue"])]),_:1},8,["open"])}}},Y=A(X,[["__scopeId","data-v-5adf1114"]]),ee={class:"networkConfig-container"},te={key:0},ae={key:1},se={key:2},ne={__name:"index",setup(M){const{SEND_MESSAGE:w}=F(),m=s([]),n=s([]),o=s({}),f=s(0),C=J(()=>({total:f.value,current:k.value,pageSize:D.value})),u=s(!1),N=e=>{o.value=e,n.value=m.value.filter(a=>{const i=e.name?a.name.includes(e.name):!0,q=e.status!==void 0?a.status===Number(e.status):!0;return i&&q})},g=()=>{const e={type:"getAiBoxNetwork"};S.on(e.type,a=>{a.result===0?(m.value=a.data.Interfaces,n.value=[...m.value]):V.error(a.msg),u.value=!1,S.off(e.type)}),u.value=!0,w(e)},{run:I,current:k,pageSize:D}=P(g,{formatResult:e=>e.data.results,pagination:{currentKey:"page",pageSizeKey:"results"}}),v=s(""),y=e=>{v.value.showDrawer(e)},t=(e,a,i)=>{console.log("🚀 ~ handleTableChange ~ pag, filters, sorter:",e,a,i),I({results:e.pageSize,page:e==null?void 0:e.current,sortField:i.field,sortOrder:i.order,...a})},l=[{type:"input",key:"name",label:"网络名称"},{type:"select",key:"status",label:"连接状态",list:[{value:"0",label:"正常"},{value:"1",label:"异常"}]}],E=[{title:"网络名称",dataIndex:"name",key:"name"},{title:"网络地址",dataIndex:"address",key:"address"},{title:"子网掩码",dataIndex:"mask",key:"mask"},{title:"网关地址",dataIndex:"gateway",key:"gateway"},{title:"物理地址",dataIndex:"mac",key:"mac"},{title:"分配方式",dataIndex:"dhcp",key:"dhcp",width:180},{title:"DNS",dataIndex:"dns",key:"dns"},{title:"连接状态",key:"status",dataIndex:"status"},{title:"操作",key:"action"}],c=s({x:"100%",y:document.body.clientHeight-380});return K(()=>{window.onresize=()=>(()=>{c.value.y=document.body.clientHeight-380})()}),(e,a)=>{const i=U,q=L,H=Z;return _(),h("div",ee,[r(O,{searchList:l,data:{name:"",status:null},onChange:N}),r(H,{columns:E,"data-source":p(n),pagination:p(C),loading:p(u),onChange:t,scroll:p(c)},{bodyCell:d(({column:B,record:b})=>[B.key==="status"?(_(),h("span",te,[r(i,{color:b.status?"volcano":"green"},{default:d(()=>[x(z(b.status?"异常":"正常"),1)]),_:2},1032,["color"])])):T("",!0),B.key==="dhcp"?(_(),h("span",ae,z(b.dhcp?"自动":"手动"),1)):T("",!0),B.key==="action"?(_(),h("span",se,[r(q,{type:"link",onClick:oe=>y(b)},{default:d(()=>a[0]||(a[0]=[x("编辑")])),_:2},1032,["onClick"])])):T("",!0)]),_:1},8,["data-source","pagination","loading","scroll"]),r(Y,{ref_key:"editRef",ref:v,onSuccess:g},null,512)])}}},ve=A(ne,[["__scopeId","data-v-723c5c54"]]);export{ve as default};