import sys
from gi.repository import EDataServer

def _listSources():
    registry  = EDataServer.SourceRegistry.new_sync()
    sources = registry.list_sources()
    for source in sources:
        print source.get_display_name(), source.get_parent()

def _createAggregatorSource():
    source = EDataServer.Source.new()
    source.set_display_name("AggregatorDisplayName")
    source.set_parent("contacts-stub")
   
    ext = source.get_extension("Address Book")
    ext.set_backend_name("aggregator")

    registry  = EDataServer.SourceRegistry.new_sync()
    registry.commit_source_sync(source, None)
        

if __name__ == "__main__":
    _createAggregatorSource()

