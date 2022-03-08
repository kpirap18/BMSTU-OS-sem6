    select o.id
	     , o.name
		 , o.price
		 , s.name
      from psql.shop.orders as o
inner join mysql.shop.sellers as s
        on o.seller = s.id
       and o.`close`
       and s.verified
