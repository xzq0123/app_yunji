import{u as V,_ as B}from"./index.es-C6lXCZft.js";import{W as L}from"./WForm-ky_TTvqU.js";import{_ as v,a as f,o as k,d as y,c as l,w as u,e as b,f as c,a1 as h,b as M,g as S,V as z,B as E}from"./index-T6jVmJWE.js";import{M as I}from"./index-BOWrvYz5.js";import"./Col-8k6je_vT.js";import"./index-DXzHBzYR.js";import"./debounce-BNUqQ2Lb.js";import"./List-37MBTKWb.js";import"./index-DzFj7X2A.js";import"./index-DsyquqNL.js";import"./ActionButton-BCuwwcwB.js";const P={class:"fixtureConfig-edit"},R={class:"edit-container"},J={__name:"Edit",setup(C,{expose:d}){const t=f(!1),o=f({}),p=i=>{console.log(i),t.value=!1},m=[{type:"textarea",label:"参数描述",key:"name"},{type:"input",label:"参数值",key:"name"}],_={name:[{required:!0,message:"请输入",trigger:"blur"}]};return d({showModal:()=>{t.value=!0}}),(i,a)=>{const g=I;return k(),y("div",P,[l(g,{open:c(t),"onUpdate:open":a[1]||(a[1]=n=>h(t)?t.value=n:null),width:"50%",onOk:p},{title:u(()=>a[2]||(a[2]=[b("div",{class:"edit-header"},"参数编辑",-1)])),default:u(()=>[b("div",R,[l(L,{modelValue:c(o),"onUpdate:modelValue":a[0]||(a[0]=n=>h(o)?o.value=n:null),formList:m,rules:_,labelWidth:"80px"},null,8,["modelValue"])])]),_:1},8,["open"])])}}},O=v(J,[["__scopeId","data-v-b21d660a"]]),T={class:"fixtureConfig-container"},W={key:0},$={__name:"index",setup(C){const d=f(""),t=()=>{d.value.showModal()},o=e=>{},{run:p,loading:m,current:_,pageSize:x}=V(o,{formatResult:e=>e.data.results,pagination:{currentKey:"page",pageSizeKey:"results"}}),i=M(()=>({total:200,current:_.value,pageSize:x.value})),a=(e,s,r)=>{console.log("🚀 ~ handleTableChange ~ pag, filters, sorter:",e,s,r),p({results:e.pageSize,page:e==null?void 0:e.current,sortField:r.field,sortOrder:r.order,...s})},g=[{title:"参数描述",dataIndex:"age",key:"age"},{title:"参数值",dataIndex:"age",key:"age"},{title:"操作",key:"action"}],n=[{key:"1",name:"John Brown",age:"/data/bm-app",address:"New York No. 1 Lake Park",tags:1},{key:"2",name:"Jim Green",age:"/data/bm-app",address:"London No. 1 Lake Park",tags:1},{key:"3",name:"Joe Black",age:32,address:"Sidney No. 1 Lake Park",tags:0}];return(e,s)=>{const r=E,w=B;return k(),y("div",T,[l(w,{columns:g,"data-source":n,pagination:c(i),loading:c(m),onChange:a,scroll:{x:!0,y:460}},{bodyCell:u(({column:N,record:q})=>[N.key==="action"?(k(),y("span",W,[l(r,{type:"link",onClick:t},{default:u(()=>s[0]||(s[0]=[S("编辑")])),_:1})])):z("",!0)]),_:1},8,["pagination","loading"]),l(O,{ref_key:"editRef",ref:d},null,512)])}}},Z=v($,[["__scopeId","data-v-39568c29"]]);export{Z as default};
