#ifndef __RA_INDEXED_CONTAINER_HPP
#define __RA_INDEXED_CONTAINER_HPP

#include <chainbase/util/object_id.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

#include <boost/core/demangle.hpp>

#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <boost/mpl/front.hpp>
#include <boost/mpl/for_each.hpp>

#include <boost/throw_exception.hpp>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <vector>

namespace steem
{
namespace chain
{
   struct by_id;
}
}

namespace chainbase
{
   using boost::multi_index::indexed_by;
   using boost::multi_index::hashed_non_unique;
   using boost::multi_index::hashed_unique;
   using boost::multi_index::ordered_unique;
   using boost::multi_index::ordered_non_unique;
   using boost::multi_index::random_access;
   using boost::multi_index::sequenced;

   using steem::chain::by_id;

namespace helpers
{
   template <typename T>
   struct ra_index_container_logging_enabled
   {
      typedef std::false_type type;
      enum
      {
         value = type::value
      };
   };

   template <typename T>
   struct ra_index_container_logging_dumper
   {
      static std::string dump(const T& v)
       {
          return "Unable to dump value";
       }
   };
   
} /// namespace helpers

   namespace detail {
      template <typename Container, typename Tag>
      class index_provider
      {
      public:
         index_provider(const typename Container::base_class& __this, const typename Container::RandomAccessContainer& rac) :_this(__this) {};
         
         index_provider(const index_provider&) = default;
         index_provider& operator =(const index_provider&) = default;

         index_provider(index_provider&&) = default;
         index_provider& operator =(index_provider&&) = default;

         const typename Container::template index< Tag >::type &get(const index_provider< Container, by_id >&) const
         {
            return _this.template get<Tag>();
         }
         typedef typename Container::template index< Tag >::type type;
   
         const typename Container::base_class& _this;
      };
   
      template <typename Container>
      class index_provider<Container, by_id >
      {
      public:
         typedef typename Container::base_class base_class;
         typedef typename base_class::const_iterator source_iterator;
         typedef typename Container::value_type      value_type;
         typedef typename Container::RandomAccessContainer RandomAccessContainer;

         index_provider(const base_class& __this, const RandomAccessContainer& rac)
            : _this(&__this), _rac(&rac), _index(__this, rac) {}

         index_provider(const index_provider&) = default;
         index_provider& operator =(const index_provider&) = default;

         index_provider(index_provider&&) = default;
         index_provider& operator =(index_provider&&) = default;

         class by_id_index
         {
         public:
            struct hole_filter
            {
               explicit hole_filter(const source_iterator& end_iterator) : _end(end_iterator) {}
               hole_filter(const hole_filter&) = default;
               hole_filter(hole_filter&&) = default;
               hole_filter& operator=(const hole_filter&) = default;
               hole_filter& operator=(hole_filter&&) = default;
               
               bool operator()(const source_iterator& item) const
               {
                   return item != _end;
               }
               
            private:
               source_iterator _end;
            };

            struct transformer
            {
               const typename Container::value_type& operator()(const source_iterator& item) const
               {
                  return *item;
               } 
            };
            
            by_id_index(const base_class& __this, const RandomAccessContainer& rac)
                : _this(&__this),
                 _rac(&rac) {}

            by_id_index(const by_id_index&) = default;
            by_id_index& operator=(const by_id_index&) = default;

            by_id_index(by_id_index&&) = default;
            by_id_index& operator=(by_id_index&&) = default;

            typedef typename boost::transform_iterator< transformer, boost::filter_iterator<hole_filter, typename RandomAccessContainer::const_iterator>, 
               const typename Container::value_type&, const typename Container::value_type > const_iterator;

            const_iterator begin() const
            {
               hole_filter filter(_this->end());
               auto source_iterator = boost::make_filter_iterator(filter, ++(_rac->begin()), _rac->end());
               return const_iterator(source_iterator, transformer());
            }            
            const_iterator end() const
            {
               hole_filter filter(_this->end());
               auto source_iterator = boost::make_filter_iterator(filter, _rac->end(), _rac->end());
               return const_iterator(source_iterator, transformer());
            }

            //template< typename ObjectType >
            const_iterator find( const oid< value_type >& key ) const
            {
               std::size_t idx = key._id;
               if(idx >= _rac->size() )
                  return this->end();
         
               hole_filter filter(_this->end());
               auto rac_it = _rac->begin() + idx;
               auto source_iterator = boost::make_filter_iterator(filter, rac_it, _rac->end());
               return const_iterator(source_iterator, transformer());
            }

         private:
            const typename Container::base_class*            _this;
            const typename Container::RandomAccessContainer* _rac;
         };

         typedef by_id_index type;

         const by_id_index &get(const index_provider< Container, by_id >& by_id_idx_provider) const
         {
            return by_id_idx_provider._index;
         }
   
      private:
         const base_class*             _this;
         const RandomAccessContainer*  _rac;
         by_id_index                   _index;
      };
   }

/** Dedicated class providing functionality of random_access (via index assigned at object emplacement)
and fast lookup, as boost::multi_index::multi_index_container does.
Supports safe object removal and reusing of previously assigned index.
*/
template <typename Value, typename IndexSpecifierList, typename Allocator = std::allocator<Value>, bool ReuseIndices = true>
class ra_indexed_container : protected boost::multi_index::multi_index_container<Value,
                                       IndexSpecifierList, Allocator>
{
public:
   typedef ra_indexed_container<Value, IndexSpecifierList, Allocator, ReuseIndices> this_type;
   template <typename Tag>
   using idx_provider = detail::index_provider<this_type, Tag>;

   typedef boost::multi_index::multi_index_container<Value, IndexSpecifierList, Allocator> base_class;
   typedef std::vector<typename base_class::const_iterator, Allocator> RandomAccessContainer;   

   using typename base_class::ctor_args_list;
   using typename base_class::index_specifier_type_list;
   using typename base_class::index_type_list;
   using typename base_class::iterator_type_list;
   using typename base_class::const_iterator_type_list;
   
   typedef typename base_class::value_type value_type;
   /// using typename base_class::value_type;
   using typename base_class::allocator_type;
   using typename base_class::iterator;
   using typename base_class::const_iterator;

   template <typename Y>
   using index=typename base_class::template index<Y>;

   using typename base_class::node_type;
  
   using base_class::cbegin;
   using base_class::cend;
   using base_class::begin;
   using base_class::end;
   using base_class::size;
   using base_class::iterator_to;
   using base_class::get_allocator;

   explicit ra_indexed_container(const allocator_type& alloc = allocator_type()) :
      base_class(alloc),
      FreeIndices(alloc),
      RandomAccessStorage(1, alloc),
      ByIdProvider(*this, RandomAccessStorage),
      LastIndex(0)
   {
   }

   ra_indexed_container(const ra_indexed_container&) = default;
   ra_indexed_container& operator=(const ra_indexed_container&) = default;
   
   ra_indexed_container(ra_indexed_container&&) = default;
   ra_indexed_container& operator=(ra_indexed_container&&) = default;
   
   const value_type *operator[](size_t idx) const
   {
      auto rasSize = RandomAccessStorage.size();

      /// \warning RA container can have gaps caused by element removal.
      if (idx < rasSize && RandomAccessStorage[idx] != end())
      {
         const auto &iterator = RandomAccessStorage[idx];
         const value_type &object = *iterator;
         return &object;
      }

      return nullptr;
   }

   const_iterator find(const oid<value_type>& id) const
   {
      std::size_t idx = id;
      if(idx >= RandomAccessStorage.size())
      {
         _log.log_find(nullptr, [&id]() {return std::to_string(id._id);});
         return cend();
      }

      auto foundPos = RandomAccessStorage[idx];
      const auto& object = *foundPos;
      _log.log_find(&object, [&id]() {return std::to_string(id._id);});
      return foundPos;
   }

   template <typename Tag>
   const typename idx_provider<Tag>::type &get() const
   {
      idx_provider<Tag> provider(*this, RandomAccessStorage);
      return provider.get(ByIdProvider);
   }

   template <typename Modifier>
   bool modify(iterator position, Modifier mod)
   {
      const value_type& object = *position;
      const auto cachedRAIdentifier = getIndex(object);

      bool result = base_class::modify(position, mod);

      if(result)
      {
         const value_type& changedObject = *position;

         const auto newRAIdentifier = getIndex(changedObject);

         if(newRAIdentifier != cachedRAIdentifier)
         {
            /// Changing object identifier is disallowed !!!
            BOOST_THROW_EXCEPTION(std::runtime_error("Cannot change persistent object id"));
         }
      }

      return result;
   }

   template <typename Constructor>
   std::pair<iterator, bool> emplace(Constructor &&c)
   {
      std::size_t index = retrieveNextId();

      auto node_ii = base_class::emplace_(c, index, this->get_allocator());

      std::pair<iterator, bool> ii = std::make_pair(const_iterator(node_ii.first), node_ii.second);

      if (node_ii.second)
      {
         auto rasSize = RandomAccessStorage.size();
         assert(index == getIndex(*ii.first) && "Built object must have assigned actual id");
         assert((ReuseIndices || index == rasSize) && "Invalid index in no-index-reusing mode");

         if (rasSize == index)
            RandomAccessStorage.push_back(ii.first);
         else
            RandomAccessStorage[index] = ii.first;

         claimNextId(index);
      }

      _log.log_emplace(ii, index, c);

      return std::move(ii);
   }

   /** Method reponsible for emplacing back all items previosly removed from the container
    *  while processing an undo state.
    *  The RA index previously associated to the source object shall be reused to satisfy
    *  referential integrity.
    */
   std::pair<iterator, bool> emplace(value_type&& source)
   {
      /// Be sure that object moved to the container storage has set correct id
      auto constructor = [&source](value_type& newObject)
      {
         newObject = std::move(source);
      };

      auto index = getIndex(source);

      auto node_ii = base_class::emplace_(constructor, index, this->get_allocator());
      std::pair<iterator, bool> ii = std::make_pair(const_iterator(node_ii.first), node_ii.second);

      if(node_ii.second == false)
      {
         BOOST_THROW_EXCEPTION(std::runtime_error("Cannot emplace object during undo state processing"));
      }

      auto rasSize = RandomAccessStorage.size();

      if(rasSize == index)
      {
         RandomAccessStorage.push_back(ii.first);
      }
      else
      {
         assert(RandomAccessStorage[index] == this->cend() &&
            "Place must point to previously erased item");
         RandomAccessStorage[index] = ii.first;
      }

      auto dummyConstructor = [](value_type&) {};

      _log.log_emplace(ii, index, dummyConstructor);

      return std::move(ii);
   }
   
   iterator erase(iterator position)
   {
      const value_type &object = *position;
      size_t index = getIndex(object);
      if( ReuseIndices )
      {
         if(index == RandomAccessStorage.size() - 1)
         {
            RandomAccessStorage.pop_back();
         }
         else
         {
            FreeIndices.emplace_back(index);
            RandomAccessStorage[index] = this->cend();
         }
      }
      else
      {
         RandomAccessStorage[index] = this->cend();
      }

      assert(RandomAccessStorage.size() >= this->size());

      _log.log_erase(position, index);

      return base_class::erase(position);
   }

   void clear()
   {
      FreeIndices.clear();
      RandomAccessStorage.clear();
      RandomAccessStorage.push_back(end());
      LastIndex = 0;
      base_class::clear();

      _log.log_clear();
   }

   size_t saveNextId() const
   {
      return LastIndex;
   }

   void restoreNextId(size_t nextId)
   {
      if(ReuseIndices == false)
      {
         assert(nextId >= this->size());
         RandomAccessStorage.resize(nextId + 1); /// plus 0 entry to match null-id
         LastIndex = nextId;
      }
   }

   private:
   /** Allows to query for the next random access identifier to be assigned to the new object being
   constructed & stored in this container.
   */
   std::size_t retrieveNextId() const
   {
      if( ReuseIndices )
         return FreeIndices.empty() ? this->size() + 1 : FreeIndices.back();
      else
         return LastIndex + 1;
   }

   /** Allows to claim previously retrieved random access indentifier to be reserved for just inserted object.
    */
   void claimNextId(std::size_t index)
   {
      if( ReuseIndices )
      {
         if (FreeIndices.empty())
         {
            assert(index == this->size() && "Container should already contain new item");
         }
         else
         {
            assert(FreeIndices.back() == index);
            FreeIndices.pop_back();
         }
      }
      else
      {
         assert(LastIndex + 1 == index);
         LastIndex = index;
      }
   }


   std::size_t getIndex(const value_type &object) const
   {
      return object.id;
   }

   /// Class members:
   private:
   typedef std::vector<std::size_t, Allocator> IndicesContainer;
   /// Stores RA-indexes of elemements being removed from container (if they didn't placed at the end of container).
   IndicesContainer FreeIndices;
   /** If at given position element has been erased, held iterator == end()
   \warning Size of this container can be bigger than size of main multi_index_container, since this one can
   store gaps previously pointing to erased objects. Such gaps will be reused an subsequent insertion.
   At the 0th element stores null element, to conform NULL_INDEX schema in the ptr_ref class.
   */
   RandomAccessContainer RandomAccessStorage = RandomAccessContainer(1, this->get_allocator());
   ///
   idx_provider< by_id > ByIdProvider;
   /// Used only in no-index-reusing mode.
   size_t LastIndex = 0;

struct index_specifier_processor
   {
   index_specifier_processor(const this_type& c, const value_type& valueToCheck, std::string& output) :
      _container(c), _value(valueToCheck), _output(output) {}

   template <typename IdxSpecType>
   void operator()(const IdxSpecType& idxSpec) const
      {
      typedef typename boost::mpl::front<typename IdxSpecType::tag_list_type>::type IndexTag;
      typedef typename IdxSpecType::key_from_value_type key_from_value_type;

      //std::string tagName = boost::core::demangle(typeid(IndexTag).name());
      std::string tagName = boost::core::demangle(typeid(IdxSpecType).name());

      const auto& idx = _container.template get<IndexTag>();
      key_from_value_type kfv;
      const auto& key = kfv(_value);

      auto ii = idx.find(key);
      if(ii != idx.end())
         _output += "Object is present in the index: `" + tagName + "'\n";
      }

   private:
      const this_type&  _container;
      const value_type& _value;
      std::string&      _output;
   };

   class logger
   {
   public:
      explicit logger(const this_type* c) : container(c) {}

      void log_find(const value_type* foundItem, std::function<std::string()> keyDump) const
      {
         if(helpers::ra_index_container_logging_enabled<value_type>::value == false)
            return;

         std::string msg = "Lookup for key: `" + keyDump() + " ";
         if(foundItem != nullptr)
         {
            msg += "succeeded. Found object:\n";
            msg += helpers::ra_index_container_logging_dumper<value_type>::dump(*foundItem);
         }
         else
         {
            msg += "failed.";
         }

         msg += '\n';
         log(msg);
      }

      template <typename ValueTypeConstructor>
      void log_emplace(std::pair<iterator, bool>& ii, size_t raIndex,
         ValueTypeConstructor&& constructor)
      {
         if(helpers::ra_index_container_logging_enabled<value_type>::value == false)
            return;

         std::string msg = "New value emplacement at RA-position: `" +
            std::to_string(raIndex) + "' ";
         if(ii.second)
         {
            msg += "succeeded. Inserted object:\n";
            msg += helpers::ra_index_container_logging_dumper<value_type>::dump(*ii.first);
         }
         else
         {
            msg += "FAILED. Conflicting object:\n";
            msg += helpers::ra_index_container_logging_dumper<value_type>::dump(*ii.first);

            value_type newObject(constructor, raIndex, container->get_allocator());
            msg += "\nNew object:\n";
            msg += helpers::ra_index_container_logging_dumper<value_type>::dump(newObject);
            msg += "\nNew object conflicts agains old one in the following indices:\n";

            index_specifier_processor processor(*container, newObject, msg);
            boost::mpl::for_each<index_specifier_type_list>(processor);
         }

         log(msg);
      }

      void log_erase(iterator itemIt, size_t raIndex)
      {
         if(helpers::ra_index_container_logging_enabled<value_type>::value == false)
            return;
         
         std::string msg("Erasing value stored at RA-position: `");
         msg += std::to_string(raIndex) + "' pointing to object:\n";
         msg += helpers::ra_index_container_logging_dumper<value_type>::dump(*itemIt);
         log(msg);
      }

      void log_clear()
      {
         if(helpers::ra_index_container_logging_enabled<value_type>::value == false)
            return;
         log("Container cleared");
      }

   private:
      void log(const std::string& text) const
      {
         if(dumpStream.is_open() == false)
         {
            dumpStream.open("ra_index_container_dump.txt");
            if(dumpStream.good())
               out = &dumpStream;
            else
               out = &std::cout;
         }
         *out << text << std::endl;
      }

   private:
      const this_type* container;
      mutable std::ofstream dumpStream;
      mutable std::ostream* out = nullptr;
   };
   
   logger _log = logger(this);
};

/** Helper trait useful to determine if specified boost::multi_index_container has defined
      MasterIndexTag (pointing to random_access index holding objects uniquely identified by their
      position in the sequence)
*/
template<typename MultiIndexType>
struct has_master_index
{
   typedef ra_indexed_container<
      typename MultiIndexType::value_type,
      typename MultiIndexType::index_specifier_type_list,
      typename MultiIndexType::allocator_type> ra_reuse;

   typedef ra_indexed_container<
      typename MultiIndexType::value_type,
      typename MultiIndexType::index_specifier_type_list,
      typename MultiIndexType::allocator_type,
      false /*No index reuse*/> ra_no_reuse;

   typedef typename std::is_same<MultiIndexType, ra_reuse>::type type_reuse;
   typedef typename std::is_same<MultiIndexType, ra_no_reuse>::type type_no_reuse;

   typedef typename std::conditional<type_reuse::value, type_reuse, type_no_reuse>::type type;

   enum { value = type::value };
};
   
} /// namespace chainbase

#endif /// __RA_INDEXED_CONTAINER_HPP
